// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Volume/VoxelVolumeSculptInnerData.h"
#include "Sculpt/Volume/VoxelVolumeChunkTree.h"
#include "Sculpt/Volume/VoxelVolumeDistanceChunk.h"
#include "Sculpt/Volume/VoxelVolumeMetadataChunk.h"
#include "Sculpt/Volume/VoxelVolumeSurfaceTypeChunk.h"
#include "Sculpt/Volume/VoxelVolumeFastDistanceChunk.h"
#include "VoxelMetadata.h"
#include "Buffer/VoxelFloatBuffers.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

FVoxelVolumeSculptInnerData::FVoxelVolumeSculptInnerData(const bool bUseFastDistances)
	: bUseFastDistances(bUseFastDistances)
	, SurfaceTypeChunkTree(MakeShared<TVoxelVolumeChunkTree<FVoxelVolumeSurfaceTypeChunk>>())
{
	if (bUseFastDistances)
	{
		ConstCast(DistanceChunkTree_LQ) = MakeShared<TVoxelVolumeChunkTree<FVoxelVolumeFastDistanceChunk>>();
	}
	else
	{
		ConstCast(DistanceChunkTree_HQ) = MakeShared<TVoxelVolumeChunkTree<FVoxelVolumeDistanceChunk>>();
	}

}

void FVoxelVolumeSculptInnerData::Serialize(FArchive& InAr)
{
	VOXEL_FUNCTION_COUNTER();

	FObjectAndNameAsStringProxyArchive StringAr(InAr, true);

	FArchive& Ar = InAr.GetArchiveName() == "FVoxelArchive" ? StringAr : InAr;

	using FVersion = DECLARE_VOXEL_VERSION
	(
		FirstVersion,
		AddDiffing,
		AddChunkTree,
		AddMetadata,
		AddSculptVersion,
		AddFastDistances
	);

	const int64 Offset = Ar.Tell();

	int32 Version = FVersion::LatestVersion;
	Ar << Version;

	bool bSerializedUseFastDistances = bUseFastDistances;
	if (Version >= FVersion::AddFastDistances)
	{
		Ar << bSerializedUseFastDistances;

		if (!ensure(bSerializedUseFastDistances == bUseFastDistances))
		{
			VOXEL_MESSAGE(Error, "bUseFastDistances mismatch");
			return;
		}
	}

	int32 SculptVersion = FVoxelVolumeSculptVersion::LatestVersion;

	if (Version < FVersion::AddSculptVersion)
	{
		SculptVersion = FVoxelVolumeSculptVersion::FirstVersion;
	}
	else
	{
		Ar << SculptVersion;
	}

	if (Version < FVersion::AddDiffing)
	{
		return;
	}

	if (Version < FVersion::AddChunkTree)
	{
		Ar.Seek(Offset);

		DistanceChunkTree_HQ->Serialize(Ar, SculptVersion);
		return;
	}

	if (bUseFastDistances)
	{
		DistanceChunkTree_LQ->Serialize(Ar, SculptVersion);
	}
	else
	{
		DistanceChunkTree_HQ->Serialize(Ar, SculptVersion);
	}

	SurfaceTypeChunkTree->Serialize(Ar, SculptVersion);

	TVoxelArray<UVoxelMetadata*> Metadatas;
	if (Ar.IsSaving())
	{
		TVoxelArray<FVoxelMetadataRef> MetadataRefs = MetadataRefToChunkTree.KeyArray();
		Metadatas.Reserve(MetadataRefs.Num());

		for (const FVoxelMetadataRef& MetadataRef : MetadataRefs)
		{
			UVoxelMetadata* Metadata = MetadataRef.GetMetadata().Resolve();
			ensureVoxelSlow(Metadata);
			Metadatas.Add_EnsureNoGrow(Metadata);
		}
	}

	if (Version < FVersion::AddMetadata)
	{
		return;
	}

	Ar << Metadatas;

	if (Ar.IsLoading())
	{
		MetadataRefToChunkTree.Reset();
		MetadataRefToChunkTree.Reserve(Metadatas.Num());

		for (UVoxelMetadata* Metadata : Metadatas)
		{
			const TSharedRef<TVoxelVolumeChunkTree<FVoxelVolumeMetadataChunk>> Tree = MakeShared<TVoxelVolumeChunkTree<FVoxelVolumeMetadataChunk>>();
			Tree->Serialize(Ar, SculptVersion);

			if (!ensureVoxelSlow(Metadata))
			{
				continue;
			}

			MetadataRefToChunkTree.Add_EnsureNew(FVoxelMetadataRef(Metadata), Tree);
		}
	}
	else
	{
		for (const auto& It : MetadataRefToChunkTree)
		{
			It.Value->Serialize(Ar, SculptVersion);
		}
	}
}

void FVoxelVolumeSculptInnerData::CopyFrom(const FVoxelVolumeSculptInnerData& Other)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(bUseFastDistances == Other.bUseFastDistances))
	{
		return;
	}

	if (bUseFastDistances)
	{
		DistanceChunkTree_LQ->CopyFrom(*Other.DistanceChunkTree_LQ);
	}
	else
	{
		DistanceChunkTree_HQ->CopyFrom(*Other.DistanceChunkTree_HQ);
	}

	SurfaceTypeChunkTree->CopyFrom(*Other.SurfaceTypeChunkTree);

	MetadataRefToChunkTree.Reset();
	MetadataRefToChunkTree.Reserve(Other.MetadataRefToChunkTree.Num());

	for (const auto& It : Other.MetadataRefToChunkTree)
	{
		const TSharedRef<TVoxelVolumeChunkTree<FVoxelVolumeMetadataChunk>> Tree = MakeShared<TVoxelVolumeChunkTree<FVoxelVolumeMetadataChunk>>();
		Tree->CopyFrom(*It.Value);
		MetadataRefToChunkTree.Add_EnsureNew(It.Key, Tree);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelVolumeSculptInnerData::ApplyShape(
	const FVoxelVolumeBulkQuery& Query,
	const FVoxelVolumeTransform& StampToQuery,
	const float Scale,
	const EVoxelVolumeBlendMode BlendMode,
	const bool bApplyOnVoid) const
{
	if (bUseFastDistances)
	{
		ApplyShape_LQ(Query, StampToQuery, Scale, BlendMode, bApplyOnVoid);
	}
	else
	{
		ApplyShape_HQ(Query, StampToQuery, Scale, BlendMode, bApplyOnVoid);
	}
}

void FVoxelVolumeSculptInnerData::ApplyShape(
	const FVoxelVolumeSparseQuery& Query,
	const FVoxelVolumeTransform& StampToQuery,
	const float Scale,
	const EVoxelVolumeBlendMode BlendMode,
	const bool bApplyOnVoid) const
{
	if (bUseFastDistances)
	{
		ApplyShape_LQ(Query, StampToQuery, Scale, BlendMode, bApplyOnVoid);
	}
	else
	{
		ApplyShape_HQ(Query, StampToQuery, Scale, BlendMode, bApplyOnVoid);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelVolumeSculptInnerData::ApplyShape_HQ(
	const FVoxelVolumeBulkQuery& Query,
	const FVoxelVolumeTransform& StampToQuery,
	const float Scale,
	const EVoxelVolumeBlendMode BlendMode,
	const bool bApplyOnVoid) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Query.Num());

	const FVoxelBox Bounds = StampToQuery.InverseTransform(Query.GetBounds()).Scale(1. / Scale);

	if (!DistanceChunkTree_HQ->HasChunks(Bounds))
	{
		if (BlendMode == EVoxelVolumeBlendMode::Intersect)
		{
			for (int32 IndexZ = Query.Indices.Min.Z; IndexZ < Query.Indices.Max.Z; IndexZ++)
			{
				for (int32 IndexY = Query.Indices.Min.Y; IndexY < Query.Indices.Max.Y; IndexY++)
				{
					for (int32 IndexX = Query.Indices.Min.X; IndexX < Query.Indices.Max.X; IndexX++)
					{
						const int32 Index = Query.GetIndex(IndexX, IndexY, IndexZ);

						Query.Distances[Index] = FVoxelUtilities::NaNf();
					}
				}
			}
		}
		return;
	}

	const TVoxelVolumeChunkTreeIterator<FVoxelVolumeDistanceChunk> Iterator = DistanceChunkTree_HQ->CreateIterator(
		Query,
		StampToQuery,
		Scale);

	ProcessDistances_HQ(
		Iterator,
		[&](
			const int32 Index,
			float AdditiveDistance,
			float SubtractiveDistance)
		{
			// AdditiveDistance will be NaN if we only removed
			// SubtractiveDistance will be NaN if we only added

			if (!FVoxelUtilities::IsNaN(AdditiveDistance))
			{
				AdditiveDistance = StampToQuery.TransformDistance(AdditiveDistance * Scale);
			}
			if (!FVoxelUtilities::IsNaN(SubtractiveDistance))
			{
				SubtractiveDistance = StampToQuery.TransformDistance(-SubtractiveDistance * Scale);
			}

			float& Distance = Query.Distances[Index];

			Distance = FVoxelVolumeDistanceChunk::Blend(
				Distance,
				AdditiveDistance,
				SubtractiveDistance,
				BlendMode,
				bApplyOnVoid);
		},
		[&](const int32 Index)
		{
			if (BlendMode == EVoxelVolumeBlendMode::Intersect)
			{
				Query.Distances[Index] = FVoxelUtilities::NaNf();
			}
		});
}

void FVoxelVolumeSculptInnerData::ApplyShape_HQ(
	const FVoxelVolumeSparseQuery& Query,
	const FVoxelVolumeTransform& StampToQuery,
	const float Scale,
	const EVoxelVolumeBlendMode BlendMode,
	const bool bApplyOnVoid) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Query.Num());

	const FVoxelBox Bounds = StampToQuery.InverseTransform(Query.PositionBounds).Scale(1. / Scale);

	if (!DistanceChunkTree_HQ->HasChunks(Bounds))
	{
		if (BlendMode == EVoxelVolumeBlendMode::Intersect)
		{
			for (int32 Index = 0; Index < Query.Num(); Index++)
			{
				Query.GetDistanceRef(Index) = FVoxelUtilities::NaNf();
			}
		}
		return;
	}

	const TVoxelVolumeChunkTreeIterator<FVoxelVolumeDistanceChunk> Iterator = DistanceChunkTree_HQ->CreateIterator(
		Query,
		StampToQuery,
		Scale);

	ProcessDistances_HQ(
		Iterator,
		[&](
			const int32 Index,
			float AdditiveDistance,
			float SubtractiveDistance)
		{
			// AdditiveDistance will be NaN if we only removed
			// SubtractiveDistance will be NaN if we only added

			if (!FVoxelUtilities::IsNaN(AdditiveDistance))
			{
				AdditiveDistance = StampToQuery.TransformDistance(AdditiveDistance * Scale);
			}
			if (!FVoxelUtilities::IsNaN(SubtractiveDistance))
			{
				SubtractiveDistance = StampToQuery.TransformDistance(-SubtractiveDistance * Scale);
			}

			float& Distance = Query.IndirectDistances[Index];

			Distance = FVoxelVolumeDistanceChunk::Blend(
				Distance,
				AdditiveDistance,
				SubtractiveDistance,
				BlendMode,
				bApplyOnVoid);
		},
		[&](const int32 Index)
		{
			if (BlendMode == EVoxelVolumeBlendMode::Intersect)
			{
				Query.IndirectDistances[Index] = FVoxelUtilities::NaNf();
			}
		});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelVolumeSculptInnerData::ApplyShape_LQ(
	const FVoxelVolumeBulkQuery& Query,
	const FVoxelVolumeTransform& StampToQuery,
	const float Scale,
	const EVoxelVolumeBlendMode BlendMode,
	const bool bApplyOnVoid) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Query.Num());

	const FVoxelBox Bounds = StampToQuery.InverseTransform(Query.GetBounds()).Scale(1. / Scale);

	if (!DistanceChunkTree_LQ->HasChunks(Bounds))
	{
		if (BlendMode == EVoxelVolumeBlendMode::Intersect)
		{
			for (int32 IndexZ = Query.Indices.Min.Z; IndexZ < Query.Indices.Max.Z; IndexZ++)
			{
				for (int32 IndexY = Query.Indices.Min.Y; IndexY < Query.Indices.Max.Y; IndexY++)
				{
					for (int32 IndexX = Query.Indices.Min.X; IndexX < Query.Indices.Max.X; IndexX++)
					{
						const int32 Index = Query.GetIndex(IndexX, IndexY, IndexZ);

						Query.Distances[Index] = FVoxelUtilities::NaNf();
					}
				}
			}
		}
		return;
	}

	const TVoxelVolumeChunkTreeIterator<FVoxelVolumeFastDistanceChunk> Iterator = DistanceChunkTree_LQ->CreateIterator(
		Query,
		StampToQuery,
		Scale);

	ProcessDistances_LQ(
		Iterator,
		[&](
			const int32 Index,
			const float FastDistance)
		{
			const float NewDistance = StampToQuery.TransformDistance(FastDistance * Scale);

			float& Distance = Query.Distances[Index];

			Distance = FVoxelVolumeFastDistanceChunk::Blend(
				Distance,
				NewDistance,
				BlendMode,
				bApplyOnVoid);
		},
		[&](const int32 Index)
		{
			if (BlendMode == EVoxelVolumeBlendMode::Intersect)
			{
				Query.Distances[Index] = FVoxelUtilities::NaNf();
			}
		});
}

void FVoxelVolumeSculptInnerData::ApplyShape_LQ(
	const FVoxelVolumeSparseQuery& Query,
	const FVoxelVolumeTransform& StampToQuery,
	const float Scale,
	const EVoxelVolumeBlendMode BlendMode,
	const bool bApplyOnVoid) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Query.Num());

	const FVoxelBox Bounds = StampToQuery.InverseTransform(Query.PositionBounds).Scale(1. / Scale);

	if (!DistanceChunkTree_LQ->HasChunks(Bounds))
	{
		if (BlendMode == EVoxelVolumeBlendMode::Intersect)
		{
			for (int32 Index = 0; Index < Query.Num(); Index++)
			{
				Query.GetDistanceRef(Index) = FVoxelUtilities::NaNf();
			}
		}
		return;
	}

	const TVoxelVolumeChunkTreeIterator<FVoxelVolumeFastDistanceChunk> Iterator = DistanceChunkTree_LQ->CreateIterator(
		Query,
		StampToQuery,
		Scale);

	ProcessDistances_LQ(
		Iterator,
		[&](
			const int32 Index,
			const float FastDistance)
		{
			const float NewDistance = StampToQuery.TransformDistance(FastDistance * Scale);

			float& Distance = Query.IndirectDistances[Index];

			Distance = FVoxelVolumeFastDistanceChunk::Blend(
				Distance,
				NewDistance,
				BlendMode,
				bApplyOnVoid);
		},
		[&](const int32 Index)
		{
			if (BlendMode == EVoxelVolumeBlendMode::Intersect)
			{
				Query.IndirectDistances[Index] = FVoxelUtilities::NaNf();
			}
		});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelVolumeSculptInnerData::ApplySurfaceType(
	const FVoxelVolumeSparseQuery& Query,
	const FVoxelVolumeTransform& StampToQuery,
	const float Scale) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Query.Num());
	ensure(Query.bQuerySurfaceTypes);

	const FVoxelBox Bounds = StampToQuery.InverseTransform(Query.PositionBounds).Scale(1. / Scale);

	if (!SurfaceTypeChunkTree->HasChunks(Bounds))
	{
		return;
	}

	const TVoxelVolumeChunkTreeIterator<FVoxelVolumeSurfaceTypeChunk> Iterator = SurfaceTypeChunkTree->CreateIterator(
		Query,
		StampToQuery,
		Scale);

	ProcessSurfaceTypes(Iterator, [&](
		const int32 Index,
		const float Alpha,
		const FVoxelSurfaceTypeBlend& SurfaceType)
	{
		if (Alpha == 0)
		{
			return;
		}

		FVoxelSurfaceTypeBlend& SurfaceTypeRef = Query.IndirectSurfaceTypes[Index];

		FVoxelSurfaceTypeBlend::Lerp(
			SurfaceTypeRef,
			SurfaceTypeRef,
			SurfaceType,
			Alpha);
	});
}

void FVoxelVolumeSculptInnerData::ApplyMetadata(
	const FVoxelMetadataRef& MetadataRef,
	const FVoxelVolumeSparseQuery& Query,
	const FVoxelVolumeTransform& StampToQuery,
	const float Scale) const
{
	const TSharedPtr<TVoxelVolumeChunkTree<FVoxelVolumeMetadataChunk>> Tree = MetadataRefToChunkTree.FindRef(MetadataRef);
	if (!Tree)
	{
		return;
	}

	VOXEL_FUNCTION_COUNTER_NUM(Query.Num());
	ensure(Query.MetadatasToQuery.Contains(MetadataRef));

	const FVoxelBox Bounds = StampToQuery.InverseTransform(Query.PositionBounds).Scale(1. / Scale);
	if (!Tree->HasChunks(Bounds))
	{
		return;
	}

	const TVoxelVolumeChunkTreeIterator<FVoxelVolumeMetadataChunk> Iterator = Tree->CreateIterator(
		Query,
		StampToQuery,
		Scale);

	if (MetadataRef.GetInnerType().Is<float>())
	{
		FVoxelFloatBuffer& Buffer = Query.IndirectMetadata.FindChecked<FVoxelFloatBuffer>(MetadataRef);

		ProcessMetadatas<float>(Iterator, [&](
			const int32 Index,
			const float Alpha,
			const float Value)
		{
			Buffer.Set(Index, FMath::Lerp(Buffer[Index], Value, Alpha));
		});
		return;
	}

	if (MetadataRef.GetInnerType().Is<FLinearColor>())
	{
		FVoxelLinearColorBuffer& Buffer = Query.IndirectMetadata.FindChecked<FVoxelLinearColorBuffer>(MetadataRef);

		ProcessMetadatas<FLinearColor>(Iterator, [&](
			const int32 Index,
			const float Alpha,
			const FLinearColor& Value)
		{
			Buffer.Set(Index, FMath::Lerp(Buffer[Index], Value, Alpha));
		});
		return;
	}

	ensureVoxelSlow(false);
	VOXEL_MESSAGE(Error, "Unsupport metadata type for sculpting: {0}", MetadataRef.GetInnerType().ToString());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelVolumeSculptInnerData::GetSurfaceTypes(
	const FVoxelDoubleVectorBuffer& Positions,
	TVoxelArray<float>& OutAlphas,
	TVoxelArray<FVoxelSurfaceTypeBlend>& OutSurfaceTypes) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Positions.Num());

	FVoxelUtilities::SetNumFast(OutAlphas, Positions.Num());
	FVoxelUtilities::SetNumFast(OutSurfaceTypes, Positions.Num());

	const TVoxelVolumeChunkTreeIterator<FVoxelVolumeSurfaceTypeChunk> Iterator = SurfaceTypeChunkTree->CreateIterator(Positions);

	ProcessSurfaceTypes(Iterator, [&](
		const int32 Index,
		const float Alpha,
		const FVoxelSurfaceTypeBlend& SurfaceType)
	{
		OutAlphas[Index] = Alpha;
		OutSurfaceTypes[Index] = SurfaceType;
	});
}

void FVoxelVolumeSculptInnerData::GetMetadatas(
	const FVoxelMetadataRef& MetadataRef,
	const FVoxelDoubleVectorBuffer& Positions,
	TVoxelArray<float>& OutAlphas,
	TSharedPtr<FVoxelBuffer>& OutMetadatas) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Positions.Num());

	FVoxelUtilities::SetNumZeroed(OutAlphas, Positions.Num());

	OutMetadatas = MetadataRef.MakeDefaultBuffer(Positions.Num());

	const TSharedPtr<TVoxelVolumeChunkTree<FVoxelVolumeMetadataChunk>> Tree = MetadataRefToChunkTree.FindRef(MetadataRef);
	if (!Tree)
	{
		return;
	}

	const TVoxelVolumeChunkTreeIterator<FVoxelVolumeMetadataChunk> Iterator = Tree->CreateIterator(Positions);

	if (MetadataRef.GetInnerType().Is<float>())
	{
		FVoxelFloatBuffer& Buffer = OutMetadatas->AsChecked<FVoxelFloatBuffer>();

		ProcessMetadatas<float>(Iterator, [&](
			const int32 Index,
			const float Alpha,
			const float Value)
		{
			OutAlphas[Index] = Alpha;
			Buffer.Set(Index, Value);
		});
		return;
	}

	if (MetadataRef.GetInnerType().Is<FLinearColor>())
	{
		FVoxelLinearColorBuffer& Buffer = OutMetadatas->AsChecked<FVoxelLinearColorBuffer>();

		ProcessMetadatas<FLinearColor>(Iterator, [&](
			const int32 Index,
			const float Alpha,
			const FLinearColor& Value)
		{
			OutAlphas[Index] = Alpha;
			Buffer.Set(Index, Value);
		});
		return;
	}

	ensureVoxelSlow(false);
	VOXEL_MESSAGE(Error, "Unsupport metadata type for sculpting: {0}", MetadataRef.GetInnerType().ToString());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename LambdaType, typename SkipLambdaType>
void FVoxelVolumeSculptInnerData::ProcessDistances_HQ(
	const TVoxelVolumeChunkTreeIterator<FVoxelVolumeDistanceChunk>& Iterator,
	LambdaType Lambda,
	SkipLambdaType SkipLambda)
{
	Iterator.Iterate(
		[&](const int32 Index,
			const FVoxelVolumeDistanceChunk* Chunk0, const int32 Index0)
		{
			if (!Chunk0)
			{
				SkipLambda(Index);
				return;
			}

			Lambda(
				Index,
				Chunk0->AdditiveDistances[Index0],
				Chunk0->SubtractiveDistances[Index0]);
		},
		[&](const int32 Index,
			const FVoxelVolumeDistanceChunk* Chunk0, const int32 Index0,
			const FVoxelVolumeDistanceChunk* Chunk1, const int32 Index1,
			const float AlphaX)
		{
			if (!Chunk0 ||
				!Chunk1)
			{
				SkipLambda(Index);
				return;
			}

			Lambda(
				Index,
				FMath::Lerp(
					Chunk0->AdditiveDistances[Index0],
					Chunk1->AdditiveDistances[Index1],
					AlphaX),
				FMath::Lerp(
					Chunk0->SubtractiveDistances[Index0],
					Chunk1->SubtractiveDistances[Index1],
					AlphaX));
		},
		[&](const int32 Index,
			const FVoxelVolumeDistanceChunk* Chunk0, const int32 Index0,
			const FVoxelVolumeDistanceChunk* Chunk1, const int32 Index1,
			const FVoxelVolumeDistanceChunk* Chunk2, const int32 Index2,
			const FVoxelVolumeDistanceChunk* Chunk3, const int32 Index3,
			const float AlphaX,
			const float AlphaY)
		{
			if (!Chunk0 ||
				!Chunk1 ||
				!Chunk2 ||
				!Chunk3)
			{
				SkipLambda(Index);
				return;
			}

			Lambda(
				Index,
				FVoxelUtilities::BilinearInterpolation(
					Chunk0->AdditiveDistances[Index0],
					Chunk1->AdditiveDistances[Index1],
					Chunk2->AdditiveDistances[Index2],
					Chunk3->AdditiveDistances[Index3],
					AlphaX,
					AlphaY),
				FVoxelUtilities::BilinearInterpolation(
					Chunk0->SubtractiveDistances[Index0],
					Chunk1->SubtractiveDistances[Index1],
					Chunk2->SubtractiveDistances[Index2],
					Chunk3->SubtractiveDistances[Index3],
					AlphaX,
					AlphaY));
		},
		[&](const int32 Index,
			const FVoxelVolumeDistanceChunk* Chunk0, const int32 Index0,
			const FVoxelVolumeDistanceChunk* Chunk1, const int32 Index1,
			const FVoxelVolumeDistanceChunk* Chunk2, const int32 Index2,
			const FVoxelVolumeDistanceChunk* Chunk3, const int32 Index3,
			const FVoxelVolumeDistanceChunk* Chunk4, const int32 Index4,
			const FVoxelVolumeDistanceChunk* Chunk5, const int32 Index5,
			const FVoxelVolumeDistanceChunk* Chunk6, const int32 Index6,
			const FVoxelVolumeDistanceChunk* Chunk7, const int32 Index7,
			const float AlphaX,
			const float AlphaY,
			const float AlphaZ)
		{
			if (!Chunk0 ||
				!Chunk1 ||
				!Chunk2 ||
				!Chunk3 ||
				!Chunk4 ||
				!Chunk5 ||
				!Chunk6 ||
				!Chunk7)
			{
				SkipLambda(Index);
				return;
			}

			Lambda(
				Index,
				FVoxelUtilities::TrilinearInterpolation(
					Chunk0->AdditiveDistances[Index0],
					Chunk1->AdditiveDistances[Index1],
					Chunk2->AdditiveDistances[Index2],
					Chunk3->AdditiveDistances[Index3],
					Chunk4->AdditiveDistances[Index4],
					Chunk5->AdditiveDistances[Index5],
					Chunk6->AdditiveDistances[Index6],
					Chunk7->AdditiveDistances[Index7],
					AlphaX,
					AlphaY,
					AlphaZ),
				FVoxelUtilities::TrilinearInterpolation(
					Chunk0->SubtractiveDistances[Index0],
					Chunk1->SubtractiveDistances[Index1],
					Chunk2->SubtractiveDistances[Index2],
					Chunk3->SubtractiveDistances[Index3],
					Chunk4->SubtractiveDistances[Index4],
					Chunk5->SubtractiveDistances[Index5],
					Chunk6->SubtractiveDistances[Index6],
					Chunk7->SubtractiveDistances[Index7],
					AlphaX,
					AlphaY,
					AlphaZ));
		});
}

template<typename LambdaType, typename SkipLambdaType>
void FVoxelVolumeSculptInnerData::ProcessDistances_LQ(
	const TVoxelVolumeChunkTreeIterator<FVoxelVolumeFastDistanceChunk>& Iterator,
	LambdaType Lambda,
	SkipLambdaType SkipLambda)
{
	Iterator.Iterate(
		[&](const int32 Index,
			const FVoxelVolumeFastDistanceChunk* Chunk0, const int32 Index0)
		{
			if (!Chunk0)
			{
				SkipLambda(Index);
				return;
			}

			Lambda(
				Index,
				Chunk0->GetDistance(Index0));
		},
		[&](const int32 Index,
			const FVoxelVolumeFastDistanceChunk* Chunk0, const int32 Index0,
			const FVoxelVolumeFastDistanceChunk* Chunk1, const int32 Index1,
			const float AlphaX)
		{
			if (!Chunk0 ||
				!Chunk1)
			{
				SkipLambda(Index);
				return;
			}

			Lambda(
				Index,
				FMath::Lerp(
					Chunk0->GetDistance(Index0),
					Chunk1->GetDistance(Index1),
					AlphaX));
		},
		[&](const int32 Index,
			const FVoxelVolumeFastDistanceChunk* Chunk0, const int32 Index0,
			const FVoxelVolumeFastDistanceChunk* Chunk1, const int32 Index1,
			const FVoxelVolumeFastDistanceChunk* Chunk2, const int32 Index2,
			const FVoxelVolumeFastDistanceChunk* Chunk3, const int32 Index3,
			const float AlphaX,
			const float AlphaY)
		{
			if (!Chunk0 ||
				!Chunk1 ||
				!Chunk2 ||
				!Chunk3)
			{
				SkipLambda(Index);
				return;
			}

			Lambda(
				Index,
				FVoxelUtilities::BilinearInterpolation(
					Chunk0->GetDistance(Index0),
					Chunk1->GetDistance(Index1),
					Chunk2->GetDistance(Index2),
					Chunk3->GetDistance(Index3),
					AlphaX,
					AlphaY));
		},
		[&](const int32 Index,
			const FVoxelVolumeFastDistanceChunk* Chunk0, const int32 Index0,
			const FVoxelVolumeFastDistanceChunk* Chunk1, const int32 Index1,
			const FVoxelVolumeFastDistanceChunk* Chunk2, const int32 Index2,
			const FVoxelVolumeFastDistanceChunk* Chunk3, const int32 Index3,
			const FVoxelVolumeFastDistanceChunk* Chunk4, const int32 Index4,
			const FVoxelVolumeFastDistanceChunk* Chunk5, const int32 Index5,
			const FVoxelVolumeFastDistanceChunk* Chunk6, const int32 Index6,
			const FVoxelVolumeFastDistanceChunk* Chunk7, const int32 Index7,
			const float AlphaX,
			const float AlphaY,
			const float AlphaZ)
		{
			if (!Chunk0 ||
				!Chunk1 ||
				!Chunk2 ||
				!Chunk3 ||
				!Chunk4 ||
				!Chunk5 ||
				!Chunk6 ||
				!Chunk7)
			{
				SkipLambda(Index);
				return;
			}

			Lambda(
				Index,
				FVoxelUtilities::TrilinearInterpolation(
					Chunk0->GetDistance(Index0),
					Chunk1->GetDistance(Index1),
					Chunk2->GetDistance(Index2),
					Chunk3->GetDistance(Index3),
					Chunk4->GetDistance(Index4),
					Chunk5->GetDistance(Index5),
					Chunk6->GetDistance(Index6),
					Chunk7->GetDistance(Index7),
					AlphaX,
					AlphaY,
					AlphaZ));
		});
}

template<typename LambdaType>
void FVoxelVolumeSculptInnerData::ProcessSurfaceTypes(
	const TVoxelVolumeChunkTreeIterator<FVoxelVolumeSurfaceTypeChunk>& Iterator,
	LambdaType Lambda)
{
#define Load(InIndex) \
	if (Chunk ## InIndex) \
	{ \
		Chunk ## InIndex->GetSurfaceType(Index ## InIndex, Alphas[InIndex], SurfaceTypes[InIndex]); \
	} \
	else \
	{ \
		Alphas[InIndex] = 0.f; \
		SurfaceTypes[InIndex].InitializeNull(); \
	}

	Iterator.Iterate(
		[&](const int32 Index,
			const FVoxelVolumeSurfaceTypeChunk* Chunk0, const int32 Index0)
		{
			float Alpha0 = 0.f;
			FVoxelSurfaceTypeBlend SurfaceType0;

			if (Chunk0)
			{
				Chunk0->GetSurfaceType(Index0, Alpha0, SurfaceType0);
			}

			Lambda(
				Index,
				Alpha0,
				SurfaceType0);
		},
		[&](const int32 Index,
			const FVoxelVolumeSurfaceTypeChunk* Chunk0, const int32 Index0,
			const FVoxelVolumeSurfaceTypeChunk* Chunk1, const int32 Index1,
			const float AlphaX)
		{
			TVoxelStaticArray<float, 2> Alphas{ NoInit };
			TVoxelStaticArray<FVoxelSurfaceTypeBlend, 2> SurfaceTypes{ NoInit };

			Load(0);
			Load(1);

			FVoxelSurfaceTypeBlend SurfaceType;
			FVoxelSurfaceTypeBlend::Lerp(
				SurfaceType,
				SurfaceTypes[0],
				SurfaceTypes[1],
				AlphaX);

			Lambda(
				Index,
				FMath::Lerp(
					Alphas[0],
					Alphas[1],
					AlphaX),
				SurfaceType);
		},
		[&](const int32 Index,
			const FVoxelVolumeSurfaceTypeChunk* Chunk0, const int32 Index0,
			const FVoxelVolumeSurfaceTypeChunk* Chunk1, const int32 Index1,
			const FVoxelVolumeSurfaceTypeChunk* Chunk2, const int32 Index2,
			const FVoxelVolumeSurfaceTypeChunk* Chunk3, const int32 Index3,
			const float AlphaX,
			const float AlphaY)
		{
			TVoxelStaticArray<float, 4> Alphas{ NoInit };
			TVoxelStaticArray<FVoxelSurfaceTypeBlend, 4> SurfaceTypes{ NoInit };

			Load(0);
			Load(1);
			Load(2);
			Load(3);

			FVoxelSurfaceTypeBlend SurfaceType;
			FVoxelSurfaceTypeBlend::BilinearInterpolation(
				SurfaceType,
				SurfaceTypes,
				AlphaX,
				AlphaY);

			Lambda(
				Index,
				FVoxelUtilities::BilinearInterpolation(
					Alphas[0],
					Alphas[1],
					Alphas[2],
					Alphas[3],
					AlphaX,
					AlphaY),
				SurfaceType);
		},
		[&](const int32 Index,
			const FVoxelVolumeSurfaceTypeChunk* Chunk0, const int32 Index0,
			const FVoxelVolumeSurfaceTypeChunk* Chunk1, const int32 Index1,
			const FVoxelVolumeSurfaceTypeChunk* Chunk2, const int32 Index2,
			const FVoxelVolumeSurfaceTypeChunk* Chunk3, const int32 Index3,
			const FVoxelVolumeSurfaceTypeChunk* Chunk4, const int32 Index4,
			const FVoxelVolumeSurfaceTypeChunk* Chunk5, const int32 Index5,
			const FVoxelVolumeSurfaceTypeChunk* Chunk6, const int32 Index6,
			const FVoxelVolumeSurfaceTypeChunk* Chunk7, const int32 Index7,
			const float AlphaX,
			const float AlphaY,
			const float AlphaZ)
		{
			TVoxelStaticArray<float, 8> Alphas{ NoInit };
			TVoxelStaticArray<FVoxelSurfaceTypeBlend, 8> SurfaceTypes{ NoInit };

			Load(0);
			Load(1);
			Load(2)
			Load(3);
			Load(4);
			Load(5);
			Load(6);
			Load(7);

			FVoxelSurfaceTypeBlend SurfaceType;
			FVoxelSurfaceTypeBlend::TrilinearInterpolation(
				SurfaceType,
				SurfaceTypes,
				AlphaX,
				AlphaY,
				AlphaZ);

			Lambda(
				Index,
				FVoxelUtilities::TrilinearInterpolation(
					Alphas[0],
					Alphas[1],
					Alphas[2],
					Alphas[3],
					Alphas[4],
					Alphas[5],
					Alphas[6],
					Alphas[7],
					AlphaX,
					AlphaY,
					AlphaZ),
				SurfaceType);
		});

#undef Load
}

template<typename InnerType, typename LambdaType>
void FVoxelVolumeSculptInnerData::ProcessMetadatas(
	const TVoxelVolumeChunkTreeIterator<FVoxelVolumeMetadataChunk>& Iterator,
	LambdaType Lambda)
{
	Iterator.Iterate(
		[&](const int32 Index,
			const FVoxelVolumeMetadataChunk* Chunk0, const int32 Index0)
		{
			if (!Chunk0)
			{
				return;
			}

			Lambda(
				Index,
				Chunk0->GetAlpha(Index0),
				Chunk0->GetValue<InnerType>(Index0));
		},
		[&](const int32 Index,
			const FVoxelVolumeMetadataChunk* Chunk0, const int32 Index0,
			const FVoxelVolumeMetadataChunk* Chunk1, const int32 Index1,
			const float AlphaX)
		{
			if (!Chunk0 ||
				!Chunk1)
			{
				return;
			}

			Lambda(
				Index,
				FMath::Lerp(
					Chunk0->GetAlpha(Index0),
					Chunk1->GetAlpha(Index1),
					AlphaX),
				FMath::Lerp(
					Chunk0->GetValue<InnerType>(Index0),
					Chunk1->GetValue<InnerType>(Index1),
					AlphaX));
		},
		[&](const int32 Index,
			const FVoxelVolumeMetadataChunk* Chunk0, const int32 Index0,
			const FVoxelVolumeMetadataChunk* Chunk1, const int32 Index1,
			const FVoxelVolumeMetadataChunk* Chunk2, const int32 Index2,
			const FVoxelVolumeMetadataChunk* Chunk3, const int32 Index3,
			const float AlphaX,
			const float AlphaY)
		{
			if (!Chunk0 ||
				!Chunk1 ||
				!Chunk2 ||
				!Chunk3)
			{
				return;
			}

			Lambda(
				Index,
				FVoxelUtilities::BilinearInterpolation(
					Chunk0->GetAlpha(Index0),
					Chunk1->GetAlpha(Index1),
					Chunk2->GetAlpha(Index2),
					Chunk3->GetAlpha(Index3),
					AlphaX,
					AlphaY),
				FVoxelUtilities::BilinearInterpolation(
					Chunk0->GetValue<InnerType>(Index0),
					Chunk1->GetValue<InnerType>(Index1),
					Chunk2->GetValue<InnerType>(Index2),
					Chunk3->GetValue<InnerType>(Index3),
					AlphaX,
					AlphaY));
		},
		[&](const int32 Index,
			const FVoxelVolumeMetadataChunk* Chunk0, const int32 Index0,
			const FVoxelVolumeMetadataChunk* Chunk1, const int32 Index1,
			const FVoxelVolumeMetadataChunk* Chunk2, const int32 Index2,
			const FVoxelVolumeMetadataChunk* Chunk3, const int32 Index3,
			const FVoxelVolumeMetadataChunk* Chunk4, const int32 Index4,
			const FVoxelVolumeMetadataChunk* Chunk5, const int32 Index5,
			const FVoxelVolumeMetadataChunk* Chunk6, const int32 Index6,
			const FVoxelVolumeMetadataChunk* Chunk7, const int32 Index7,
			const float AlphaX,
			const float AlphaY,
			const float AlphaZ)
		{
			if (!Chunk0 ||
				!Chunk1 ||
				!Chunk2 ||
				!Chunk3 ||
				!Chunk4 ||
				!Chunk5 ||
				!Chunk6 ||
				!Chunk7)
			{
				return;
			}

			Lambda(
				Index,
				FVoxelUtilities::TrilinearInterpolation(
					Chunk0->GetAlpha(Index0),
					Chunk1->GetAlpha(Index1),
					Chunk2->GetAlpha(Index2),
					Chunk3->GetAlpha(Index3),
					Chunk4->GetAlpha(Index4),
					Chunk5->GetAlpha(Index5),
					Chunk6->GetAlpha(Index6),
					Chunk7->GetAlpha(Index7),
					AlphaX,
					AlphaY,
					AlphaZ),
				FVoxelUtilities::TrilinearInterpolation(
					Chunk0->GetValue<InnerType>(Index0),
					Chunk1->GetValue<InnerType>(Index1),
					Chunk2->GetValue<InnerType>(Index2),
					Chunk3->GetValue<InnerType>(Index3),
					Chunk4->GetValue<InnerType>(Index4),
					Chunk5->GetValue<InnerType>(Index5),
					Chunk6->GetValue<InnerType>(Index6),
					Chunk7->GetValue<InnerType>(Index7),
					AlphaX,
					AlphaY,
					AlphaZ));
		});
}