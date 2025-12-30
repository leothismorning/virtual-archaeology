// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Heightmap/VoxelHeightmapStamp.h"
#include "Heightmap/VoxelHeightmap_Height.h"
#include "Heightmap/VoxelHeightmap_Weight.h"
#include "VoxelStampUtilities.h"
#include "Buffer/VoxelBaseBuffers.h"
#include "Surface/VoxelSurfaceTypeInterface.h"
#include "VoxelHeightmapStampImpl.ispc.generated.h"

void FVoxelHeightmapStamp::FixupProperties(const IVoxelStampComponentInterface* Interface)
{
	Super::FixupProperties(Interface);

#if WITH_EDITOR
	if (Heightmap)
	{
		Heightmap->MigrateMaterials();
	}

	UVoxelSurfaceTypeInterface::Migrate(DefaultMaterial, DefaultSurfaceType);

	for (FVoxelHeightmapStampWeightmapSurfaceType& WeightmapMaterial : WeightmapSurfaceTypes)
	{
		UVoxelSurfaceTypeInterface::Migrate(WeightmapMaterial.Material, WeightmapMaterial.SurfaceType);
	}
#endif
}

#if WITH_EDITOR
void FVoxelHeightmapStamp::GetActorLabelInfo(FActorLabelInfo& Info) const
{
	Info.BlendMode = GetEnumDisplayName(BlendMode).ToString();
	Info.Identifier = Heightmap ? Heightmap->GetName() : "None";
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelHeightmapStampRuntime::Initialize(FVoxelDependencyCollector& DependencyCollector)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	UVoxelHeightmap* Heightmap = Stamp.Heightmap;
	if (!Heightmap)
	{
		return false;
	}

#if WITH_EDITOR
	Heightmap->OnChanged_EditorOnly.Add(MakeWeakPtrDelegate(this, [this]
	{
		RequestUpdate();
	}));
#endif

	const UVoxelHeightmap_Height* Height = Heightmap->Height;
	if (!ensure(Height))
	{
		return false;
	}

	ScaleXY = Heightmap->ScaleXY;
	DefaultSurfaceType = FVoxelSurfaceType(
		Stamp.DefaultSurfaceType
		? Stamp.DefaultSurfaceType
		: Heightmap->DefaultSurfaceType);

	ScaleZ = Height->ScaleZ;
	OffsetZ = Height->OffsetZ;
	bUseBicubic = Height->bUseBicubic;

	HeightData = Height->GetData();
	MetadataOverrides = Stamp.MetadataOverrides.CreateRuntime();

	if (!HeightData)
	{
		return false;
	}

	for (int32 Index = 0; Index < Heightmap->Weights.Num(); Index++)
	{
		const UVoxelHeightmap_Weight* Weight = Heightmap->Weights[Index];
		if (!ensure(Weight))
		{
			continue;
		}

		const TSharedPtr<const FVoxelHeightmap_WeightData> WeightData = Weight->GetData();
		if (!WeightData)
		{
			continue;
		}

		UVoxelSurfaceTypeInterface* SurfaceType = Weight->SurfaceType;
		if (const FVoxelHeightmapStampWeightmapSurfaceType* WeightmapSurfaceOverride = Stamp.WeightmapSurfaceTypes.FindByPredicate(
			[&](const FVoxelHeightmapStampWeightmapSurfaceType& WeightmapSurfaceType)
			{
				return WeightmapSurfaceType.Index == Index;
			}))
		{
			if (WeightmapSurfaceOverride->SurfaceType)
			{
				SurfaceType = WeightmapSurfaceOverride->SurfaceType;
			}
		}

		Weightmaps.Add(FWeightmap
		{
			FVoxelSurfaceType(SurfaceType),
			Weight->Type,
			Weight->Strength,
			FVoxelFloatMetadataRef(Weight->Metadata),
			Weight->bUseBicubic,
			WeightData
		});
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelBox FVoxelHeightmapStampRuntime::GetLocalBounds() const
{
	const double Min = (HeightData->DataMin * HeightData->InternalScaleZ + HeightData->InternalOffsetZ) * ScaleZ + OffsetZ;
	const double Max = (HeightData->DataMax * HeightData->InternalScaleZ + HeightData->InternalOffsetZ) * ScaleZ + OffsetZ;

	const FVector2D Size
	{
		double(HeightData->SizeX),
		double(HeightData->SizeY)
	};
	return FVoxelBox2D(-Size / 2, Size / 2).Scale(ScaleXY).ToBox3D(Min, Max);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelHeightmapStampRuntime::Apply(
	const FVoxelHeightBulkQuery& Query,
	const FVoxelHeightTransform& StampToQuery) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Query.Num());
	check(HeightData->RawData.Num() == HeightData->SizeX * HeightData->SizeY * (HeightData->bIsUINT16 ? sizeof(uint16) : sizeof(float)));

	if (!AffectShape())
	{
		return;
	}

	float FinalScaleZ;
	float FinalOffsetZ;
	FVoxelUtilities::CombineScaleAndOffset(
		HeightData->InternalScaleZ,
		HeightData->InternalOffsetZ,
		ScaleZ,
		OffsetZ,
		FinalScaleZ,
		FinalOffsetZ);

	const ispc::EVoxelHeightBlendMode BlendMode = INLINE_LAMBDA
	{
		switch (Stamp.BlendMode)
		{
		default: ensure(false);
		case EVoxelHeightBlendMode::Max: return ispc::EVoxelHeightBlendMode::HeightBlendMode_Max;
		case EVoxelHeightBlendMode::Min: return ispc::EVoxelHeightBlendMode::HeightBlendMode_Min;
		case EVoxelHeightBlendMode::Override: return ispc::EVoxelHeightBlendMode::HeightBlendMode_Override;
		}
	};

	if (HeightData->bIsUINT16)
	{
		ispc::VoxelHeightmapStamp_ApplyHeightmap_uint16(
			Query.ISPC(),
			StampToQuery.ISPC(),
			BlendMode,
			Stamp.bApplyOnVoid,
			Stamp.Smoothness,
			bUseBicubic,
			FinalScaleZ / MAX_uint16,
			FinalOffsetZ,
			ScaleXY,
			HeightData->SizeX,
			HeightData->SizeY,
			HeightData->RawData.View<uint16>().GetData());
	}
	else
	{
		ispc::VoxelHeightmapStamp_ApplyHeightmap_float(
			Query.ISPC(),
			StampToQuery.ISPC(),
			BlendMode,
			Stamp.bApplyOnVoid,
			Stamp.Smoothness,
			bUseBicubic,
			FinalScaleZ,
			FinalOffsetZ,
			ScaleXY,
			HeightData->SizeX,
			HeightData->SizeY,
			HeightData->RawData.View<float>().GetData());
	}
}

void FVoxelHeightmapStampRuntime::Apply(
	const FVoxelHeightSparseQuery& Query,
	const FVoxelHeightTransform& StampToQuery) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Query.Num());
	check(HeightData->RawData.Num() == HeightData->SizeX * HeightData->SizeY * (HeightData->bIsUINT16 ? sizeof(uint16) : sizeof(float)));

	float FinalScaleZ;
	float FinalOffsetZ;
	FVoxelUtilities::CombineScaleAndOffset(
		HeightData->InternalScaleZ,
		HeightData->InternalOffsetZ,
		ScaleZ,
		OffsetZ,
		FinalScaleZ,
		FinalOffsetZ);

	const ispc::EVoxelHeightBlendMode BlendMode = INLINE_LAMBDA
	{
		switch (Stamp.BlendMode)
		{
		default: ensure(false);
		case EVoxelHeightBlendMode::Max: return ispc::EVoxelHeightBlendMode::HeightBlendMode_Max;
		case EVoxelHeightBlendMode::Min: return ispc::EVoxelHeightBlendMode::HeightBlendMode_Min;
		case EVoxelHeightBlendMode::Override: return ispc::EVoxelHeightBlendMode::HeightBlendMode_Override;
		}
	};

	const bool bComputeSurfaceTypes = INLINE_LAMBDA
	{
		if (!AffectSurfaceType() ||
			!Query.bQuerySurfaceTypes)
		{
			return false;
		}

		if (!DefaultSurfaceType.IsNull())
		{
			return true;
		}

		for (const FWeightmap& Weightmap : Weightmaps)
		{
			if (!Weightmap.SurfaceType.IsNull())
			{
				return true;
			}
		}

		return false;
	};

	const bool bComputeMetadatas = INLINE_LAMBDA
	{
		if (!AffectMetadata() ||
			Query.MetadatasToQuery.Num() == 0)
		{
			return false;
		}

		for (const FWeightmap& Weightmap : Weightmaps)
		{
			if (Query.MetadatasToQuery.Contains(Weightmap.MetadataRef))
			{
				return true;
			}
		}

		return MetadataOverrides->ShouldCompute(Query);
	};

	if (!bComputeSurfaceTypes &&
		!bComputeMetadatas)
	{
		if (AffectShape())
		{
			if (HeightData->bIsUINT16)
			{
				ispc::VoxelHeightmapStamp_ApplyHeightmap_Sparse_uint16(
					Query.ISPC(),
					StampToQuery.ISPC(),
					nullptr,
					true,
					false,
					BlendMode,
					Stamp.bApplyOnVoid,
					Stamp.Smoothness,
					bUseBicubic,
					FinalScaleZ / MAX_uint16,
					FinalOffsetZ,
					ScaleXY,
					HeightData->SizeX,
					HeightData->SizeY,
					HeightData->RawData.View<uint16>().GetData());
			}
			else
			{
				ispc::VoxelHeightmapStamp_ApplyHeightmap_Sparse_float(
					Query.ISPC(),
					StampToQuery.ISPC(),
					nullptr,
					true,
					false,
					BlendMode,
					Stamp.bApplyOnVoid,
					Stamp.Smoothness,
					bUseBicubic,
					FinalScaleZ,
					FinalOffsetZ,
					ScaleXY,
					HeightData->SizeX,
					HeightData->SizeY,
					HeightData->RawData.View<float>().GetData());
			}
		}

		return;
	}

	FVoxelFloatBuffer Alphas;
	Alphas.Allocate(Query.Num());

	if (HeightData->bIsUINT16)
	{
		ispc::VoxelHeightmapStamp_ApplyHeightmap_Sparse_uint16(
			Query.ISPC(),
			StampToQuery.ISPC(),
			Alphas.GetData(),
			AffectShape(),
			true,
			BlendMode,
			Stamp.bApplyOnVoid,
			Stamp.Smoothness,
			bUseBicubic,
			FinalScaleZ / MAX_uint16,
			FinalOffsetZ,
			ScaleXY,
			HeightData->SizeX,
			HeightData->SizeY,
			HeightData->RawData.View<uint16>().GetData());
	}
	else
	{
		ispc::VoxelHeightmapStamp_ApplyHeightmap_Sparse_float(
			Query.ISPC(),
			StampToQuery.ISPC(),
			Alphas.GetData(),
			AffectShape(),
			true,
			BlendMode,
			Stamp.bApplyOnVoid,
			Stamp.Smoothness,
			bUseBicubic,
			FinalScaleZ,
			FinalOffsetZ,
			ScaleXY,
			HeightData->SizeX,
			HeightData->SizeY,
			HeightData->RawData.View<float>().GetData());
	}

	struct FWeightData
	{
		const FWeightmap* Weightmap;
		FVoxelFloatBuffer Weights;
	};
	TVoxelInlineArray<FWeightData, 16> WeightDatas;
	WeightDatas.Reserve(Weightmaps.Num());

	for (const FWeightmap& Weightmap : Weightmaps)
	{
		const bool bShouldCompute = INLINE_LAMBDA
		{
			if (bComputeSurfaceTypes &&
				!Weightmap.SurfaceType.IsNull())
			{
				return true;
			}

			if (bComputeMetadatas &&
				Query.MetadatasToQuery.Contains(Weightmap.MetadataRef))
			{
				return true;
			}

			return false;
		};

		if (!bShouldCompute)
		{
			continue;
		}

		const FVector2f HeightmapToWeightmap = FVector2f(
			Weightmap.Data->SizeX / float(HeightData->SizeX),
			Weightmap.Data->SizeY / float(HeightData->SizeY));

		FWeightData& WeightData = WeightDatas.Emplace_GetRef_EnsureNoGrow();
		WeightData.Weightmap = &Weightmap;
		WeightData.Weights.Allocate(Query.Num());

		VOXEL_SCOPE_COUNTER("SampleWeightmap");

		ispc::VoxelHeightmapStamp_SampleWeightmap(
			Query.ISPC(),
			StampToQuery.ISPC(),
			WeightData.Weights.GetData(),
			Alphas.GetData(),
			Weightmap.Strength,
			Weightmap.bUseBicubic,
			ScaleXY,
			GetISPCValue(HeightmapToWeightmap),
			Weightmap.Data->SizeX,
			Weightmap.Data->SizeY,
			Weightmap.Data->Weights.GetData());
	}

	if (bComputeSurfaceTypes)
	{
		VOXEL_SCOPE_COUNTER("Surface types");

		struct FBlend
		{
			FVoxelSurfaceType SurfaceType;
			TVoxelInlineArray<TConstVoxelArrayView<float>, 1> AllWeights;
		};
		TVoxelInlineArray<FBlend, 16> WeightBlends;
		TVoxelInlineArray<FBlend, 16> AlphaBlends;

		{
			VOXEL_SCOPE_COUNTER("Setup blends");

			WeightBlends.Reserve(WeightDatas.Num());
			AlphaBlends.Reserve(WeightDatas.Num());

			for (const FWeightData& WeightData : WeightDatas)
			{
				const FVoxelSurfaceType SurfaceType = WeightData.Weightmap->SurfaceType;
				if (SurfaceType.IsNull())
				{
					continue;
				}

				TVoxelInlineArray<FBlend, 16>& Blends =
					WeightData.Weightmap->Type == EVoxelHeightmapWeightType::AlphaBlended
					? AlphaBlends
					: WeightBlends;

				const bool bAdded = INLINE_LAMBDA
				{
					for (FBlend& Blend : Blends)
					{
						if (Blend.SurfaceType == SurfaceType)
						{
							Blend.AllWeights.Add(WeightData.Weights.View());
							return true;
						}
					}

					return false;
				};
				if (bAdded)
				{
					continue;
				}

				FBlend& Blend = Blends.Emplace_GetRef_EnsureNoGrow(FBlend
				{
					WeightData.Weightmap->SurfaceType,
				});

				Blend.AllWeights.Add_EnsureNoGrow(WeightData.Weights.View());
			}
		}

		TVoxelArray<FVoxelSurfaceTypeBlendLayer> WeightLayers;
		FVoxelUtilities::SetNumFast(WeightLayers, WeightBlends.Num());

		for (int32 Index = 0; Index < Query.Num(); Index++)
		{
			const float Alpha = Alphas[Index];
			if (Alpha == 0.f)
			{
				continue;
			}
			checkVoxelSlow(FVoxelUtilities::IsFinite(Alpha));
			checkVoxelSlow(0.f <= Alpha && Alpha <= 1.f);

			FVoxelSurfaceTypeBlend SurfaceType;

			// Process weight-blended weightmaps
			INLINE_LAMBDA
			{
				if (WeightBlends.Num() == 0)
				{
					return;
				}

				int32 NumLayers = 0;
				for (const FBlend& Blend : WeightBlends)
				{
					float Weight = Blend.AllWeights[0][Index];
					if (Blend.AllWeights.Num() > 1)
					{
						for (int32 WeightmapIndex = 1; WeightmapIndex < Blend.AllWeights.Num(); WeightmapIndex++)
						{
							Weight += Blend.AllWeights[WeightmapIndex][Index];
						}
					}
					checkVoxelSlow(FVoxelUtilities::IsFinite(Weight));
					checkVoxelSlow(0.f <= Weight && Weight <= 1.f);

					if (Weight == 0.f)
					{
						continue;
					}

					WeightLayers[NumLayers++] = FVoxelSurfaceTypeBlendLayer(Blend.SurfaceType, Weight);
				}

				SurfaceType.InitializeFromLayers(WeightLayers.View().LeftOf(NumLayers));
			};

			if (SurfaceType.IsNull())
			{
				SurfaceType.InitializeFromType(DefaultSurfaceType);
			}

			// Process alpha-blended weightmaps
			for (const FBlend& Blend : AlphaBlends)
			{
				float Weight = Blend.AllWeights[0][Index];
				if (Blend.AllWeights.Num() > 1)
				{
					for (int32 WeightmapIndex = 1; WeightmapIndex < Blend.AllWeights.Num(); WeightmapIndex++)
					{
						Weight += Blend.AllWeights[WeightmapIndex][Index];
					}
				}
				checkVoxelSlow(FVoxelUtilities::IsFinite(Weight));
				checkVoxelSlow(0.f <= Weight && Weight <= 1.f);

				if (Weight == 0.f)
				{
					continue;
				}

				FVoxelSurfaceTypeBlend::Lerp(
					SurfaceType,
					SurfaceType,
					Blend.SurfaceType,
					Weight);
			}

			FVoxelSurfaceTypeBlend& SurfaceTypeRef = Query.IndirectSurfaceTypes[Query.GetIndirectIndex(Index)];

			FVoxelSurfaceTypeBlend::Lerp(
				SurfaceTypeRef,
				SurfaceTypeRef,
				SurfaceType,
				Alpha);
		}
	}

	if (bComputeMetadatas)
	{
		VOXEL_SCOPE_COUNTER("Metadata");

		for (const FWeightData& WeightData : WeightDatas)
		{
			if (!Query.MetadatasToQuery.Contains(WeightData.Weightmap->MetadataRef))
			{
				continue;
			}

			VOXEL_SCOPE_COUNTER_FNAME(WeightData.Weightmap->MetadataRef.GetFName());

			FVoxelFloatBuffer& IndirectBuffer = Query.IndirectMetadata.FindChecked<FVoxelFloatBuffer>(WeightData.Weightmap->MetadataRef);
			check(IndirectBuffer.Num() == Query.IndirectHeights.Num());

			ispc::VoxelHeightmapStamp_BlendMetadata(
				Query.ISPC(),
				IndirectBuffer.GetData(),
				WeightData.Weights.GetData(),
				Alphas.GetData());
		}

		FVoxelStampUtilities::ApplyMetadatas(
			Query,
			*MetadataOverrides,
			{},
			Alphas);
	}
}