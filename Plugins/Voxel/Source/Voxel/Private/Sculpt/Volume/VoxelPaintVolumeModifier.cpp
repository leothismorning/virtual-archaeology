// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Volume/VoxelPaintVolumeModifier.h"
#include "Buffer/VoxelBaseBuffers.h"

void FVoxelPaintVolumeModifier::Initialize_GameThread()
{
	Super::Initialize_GameThread();

	RuntimeSurfaceType = FVoxelSurfaceType(SurfaceTypeToPaint);
	RuntimeMetadatas = MetadatasToPaint.CreateRuntime();

	RuntimeBrush = Brush.GetRuntimeBrush();
}

FVoxelBox FVoxelPaintVolumeModifier::GetBounds() const
{
	return FVoxelBox(Center - Radius, Center + Radius);
}

void FVoxelPaintVolumeModifier::GetUsage(
	bool& bWritesDistances,
	bool& bWritesSurfaceTypes,
	TVoxelSet<FVoxelMetadataRef>& MetadataRefsToWrite) const
{
	bWritesSurfaceTypes = !RuntimeSurfaceType.IsNull();

	for (const auto& It : RuntimeMetadatas->MetadataToValue)
	{
		MetadataRefsToWrite.Add(It.Key);
	}
}

void FVoxelPaintVolumeModifier::Apply(const FData& Data) const
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelIntBox Indices = Data.Indices;

	FVoxelFloatBuffer Alphas;
	FVoxelInt32Buffer Indirection;

	Alphas.Allocate(Indices.Count_int32());
	Indirection.Allocate(Indices.Count_int32());

	FVoxelCounter32 AtomicWriteIndex;

	Voxel::ParallelFor(Indices.GetZ(), [&](const int32 IndexZ)
	{
		for (int32 IndexY = Indices.Min.Y; IndexY < Indices.Max.Y; IndexY++)
		{
			for (int32 IndexX = Indices.Min.X; IndexX < Indices.Max.X; IndexX++)
			{
				const int32 Index = IndexX + Data.Size.X * IndexY + Data.Size.X * Data.Size.Y * IndexZ;
				const FVector Position = Data.IndexToWorld.TransformPosition(FVector(IndexX, IndexY, IndexZ));

				const double DistanceToCenter = FVector::Distance(Position, Center);

				const float FalloffMultiplier = RuntimeBrush->GetFalloff(DistanceToCenter, Radius);
				const float MaskStrength = RuntimeBrush->GetMaskStrength(Center, Position, Radius);

				float Alpha = FalloffMultiplier * Strength * MaskStrength;
				Alpha = FMath::Clamp(Alpha, 0.f, 1.f);

				if (Alpha == 0.f)
				{
					continue;
				}

				if (Mode == EVoxelSculptMode::Remove)
				{
					if (Data.SurfaceTypes)
					{
						float& SurfaceTypeAlpha = Data.SurfaceTypes->Alphas[Index];

						SurfaceTypeAlpha = FMath::Lerp(
							SurfaceTypeAlpha,
							0.f,
							Alpha);
					}

					for (auto& It : Data.MetadataRefToMetadata)
					{
						float& MetadataAlpha = It.Value->Alphas[Index];

						MetadataAlpha = FMath::Lerp(
							MetadataAlpha,
							0.f,
							Alpha);
					}

					continue;
				}

				if (Data.SurfaceTypes)
				{
					float& SurfaceTypeAlpha = Data.SurfaceTypes->Alphas[Index];

					SurfaceTypeAlpha = FMath::Lerp(
						SurfaceTypeAlpha,
						1.f,
						Alpha);
				}

				for (auto& It : Data.MetadataRefToMetadata)
				{
					float& MetadataAlpha = It.Value->Alphas[Index];

					MetadataAlpha = FMath::Lerp(
						MetadataAlpha,
						1.f,
						Alpha);
				}

				const int32 WriteIndex = AtomicWriteIndex.Increment_ReturnOld();

				Alphas.Set(WriteIndex, Alpha);
				Indirection.Set(WriteIndex, Index);
			}
		}
	});

	const int32 Num = AtomicWriteIndex.Get();

	if (Num == 0)
	{
		return;
	}

	Alphas.ShrinkTo(Num);
	Indirection.ShrinkTo(Num);

	if (Data.SurfaceTypes)
	{
		VOXEL_SCOPE_COUNTER_NUM("SurfaceTypes", Num);

		Voxel::ParallelFor(Num, [&](const int32 Index)
		{
			FVoxelSurfaceTypeBlend& SurfaceTypeBlend = Data.SurfaceTypes->SurfaceTypes[Indirection[Index]];

			FVoxelSurfaceTypeBlend::Lerp(
				SurfaceTypeBlend,
				SurfaceTypeBlend,
				RuntimeSurfaceType,
				Alphas[Index]);
		});
	}

	for (const auto& It : Data.MetadataRefToMetadata)
	{
		VOXEL_SCOPE_COUNTER_FORMAT("Metadata %s Num=%d", *It.Key.GetFName().ToString(), Num);

		It.Key.IndirectBlend(
			Indirection.View(),
			*RuntimeMetadatas->MetadataToValue[It.Key].Constant,
			Alphas,
			*It.Value->Buffer);
	}
}