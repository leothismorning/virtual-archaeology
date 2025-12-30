// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Height/VoxelHeightSculptUtilities.h"
#include "Sculpt/Height/VoxelHeightHeightChunk.h"
#include "Sculpt/Height/VoxelHeightMetadataChunk.h"
#include "Sculpt/Height/VoxelHeightSurfaceTypeChunk.h"
#include "Buffer/VoxelBaseBuffers.h"

TVoxelRefCountPtr<FVoxelHeightHeightChunk> FVoxelHeightSculptUtilities::CreateHeightChunk(
	const FIntPoint& ChunkKey,
	const TConstVoxelArrayView<float> Heights,
	const FIntPoint& Size,
	const float ScaleZ,
	const float OffsetZ)
{
	VOXEL_FUNCTION_COUNTER();
	check(Heights.Num() == Size.X * Size.Y);

	TVoxelRefCountPtr<FVoxelHeightHeightChunk> Chunk = new FVoxelHeightHeightChunk();

	for (int32 IndexY = 0; IndexY < ChunkSize; IndexY++)
	{
		for (int32 IndexX = 0; IndexX < ChunkSize; IndexX++)
		{
			const int32 IndexInChunk = FVoxelUtilities::Get2DIndex<int32>(
				ChunkSize,
				IndexX,
				IndexY);

			const int32 IndexInSculpt = FVoxelUtilities::Get2DIndex<int32>(
				Size,
				ChunkKey.X * ChunkSize + IndexX,
				ChunkKey.Y * ChunkSize + IndexY);

			float Height = Heights[IndexInSculpt];

			if (!FVoxelUtilities::IsNaN(Height))
			{
				Height = (Height - OffsetZ) / ScaleZ;
			}

			Chunk->Heights[IndexInChunk] = Height;
		}
	}

	return Chunk;
}

TVoxelRefCountPtr<FVoxelHeightSurfaceTypeChunk> FVoxelHeightSculptUtilities::CreateSurfaceTypeChunk(
	const FIntPoint& ChunkKey,
	const TConstVoxelArrayView<float> Alphas,
	const TConstVoxelArrayView<FVoxelSurfaceTypeBlend> SurfaceTypes,
	const FIntPoint& Size)
{
	VOXEL_FUNCTION_COUNTER();
	check(SurfaceTypes.Num() == Size.X * Size.Y);

	TVoxelRefCountPtr<FVoxelHeightSurfaceTypeChunk> Chunk;

	{
		VOXEL_SCOPE_COUNTER("UsedSurfaceTypes");

		int32 NumLayers = 0;

		TVoxelArray<FVoxelSurfaceType> UsedSurfaceTypes;
		UsedSurfaceTypes.Reserve(16);

		for (int32 IndexY = 0; IndexY < ChunkSize; IndexY++)
		{
			for (int32 IndexX = 0; IndexX < ChunkSize; IndexX++)
			{
				const int32 Index = FVoxelUtilities::Get2DIndex<int32>(
					Size,
					ChunkKey.X * ChunkSize + IndexX,
					ChunkKey.Y * ChunkSize + IndexY);

				const FVoxelSurfaceTypeBlend& SurfaceType = SurfaceTypes[Index];

				NumLayers = FMath::Max(NumLayers, SurfaceType.GetLayers().Num());

				for (const FVoxelSurfaceTypeBlendLayer& Layer : SurfaceType.GetLayers())
				{
					UsedSurfaceTypes.AddUnique(Layer.Type);
				}
			}
		}

		if (NumLayers == 0)
		{
			return nullptr;
		}

		Chunk = new FVoxelHeightSurfaceTypeChunk();
		Chunk->Layers.SetNum(NumLayers);
		Chunk->UsedSurfaceTypes = MoveTemp(UsedSurfaceTypes);
	}

	FVoxelUtilities::Memzero_Stats(Chunk->Layers);

	for (int32 IndexY = 0; IndexY < ChunkSize; IndexY++)
	{
		for (int32 IndexX = 0; IndexX < ChunkSize; IndexX++)
		{
			const int32 Index = FVoxelUtilities::Get2DIndex<int32>(
				Size,
				ChunkKey.X * ChunkSize + IndexX,
				ChunkKey.Y * ChunkSize + IndexY);

			const int32 IndexInChunk = FVoxelUtilities::Get2DIndex<int32>(
				ChunkSize,
				IndexX,
				IndexY);

			Chunk->Alphas[IndexInChunk] = FVoxelUtilities::FloatToUINT8(Alphas[Index]);

			const FVoxelSurfaceTypeBlend& SurfaceType = SurfaceTypes[Index];

			float MaxWeight = 0.f;
			for (const FVoxelSurfaceTypeBlendLayer& Layer : SurfaceType.GetLayers())
			{
				MaxWeight = FMath::Max(MaxWeight, Layer.Weight);
			}

			int32 LayerIndex = 0;
			for (const FVoxelSurfaceTypeBlendLayer& Layer : SurfaceType.GetLayers())
			{
				FVoxelHeightSurfaceTypeChunk::FLayer& ChunkLayer = Chunk->Layers[LayerIndex++];

				const int32 TypeIndex = Chunk->UsedSurfaceTypes.Find(Layer.Type);
				checkVoxelSlow(FVoxelUtilities::IsValidUINT8(TypeIndex));

				ChunkLayer.Types[IndexInChunk] = uint8(TypeIndex);
				ChunkLayer.Weights[IndexInChunk] = FVoxelUtilities::FloatToUINT8(Layer.Weight / MaxWeight);
			}
		}
	}

	return Chunk;
}

TVoxelRefCountPtr<FVoxelHeightMetadataChunk> FVoxelHeightSculptUtilities::CreateMetadataChunk(
	const FIntPoint& ChunkKey,
	const TConstVoxelArrayView<float> Alphas,
	const FVoxelBuffer& Buffer,
	const FIntPoint& Size)
{
	VOXEL_FUNCTION_COUNTER();
	check(Buffer.Num_Slow() == Size.X * Size.Y);

	TVoxelRefCountPtr<FVoxelHeightMetadataChunk> Chunk = new FVoxelHeightMetadataChunk();

	TVoxelArray<int32> LocalIndirection;
	FVoxelUtilities::SetNumFast(LocalIndirection, ChunkCount);
	{
		VOXEL_SCOPE_COUNTER("Build indirection");

		for (int32 IndexZ = 0; IndexZ < ChunkSize; IndexZ++)
		{
			for (int32 IndexY = 0; IndexY < ChunkSize; IndexY++)
			{
				for (int32 IndexX = 0; IndexX < ChunkSize; IndexX++)
				{
					const int32 IndexInChunk = FVoxelUtilities::Get2DIndex<int32>(
						ChunkSize,
						IndexX,
						IndexY,
						IndexZ);

					const int32 Index = FVoxelUtilities::Get2DIndex<int32>(
						Size,
						ChunkKey.X * ChunkSize + IndexX,
						ChunkKey.Y * ChunkSize + IndexY);

					Chunk->Alphas[IndexInChunk] = FVoxelUtilities::FloatToUINT8(Index == -1 ? 0.f : Alphas[Index]);
					LocalIndirection[IndexInChunk] = Index;
				}
			}
		}
	}

	if (FVoxelUtilities::AllEqual(Chunk->Alphas, 0))
	{
		return nullptr;
	}

	Chunk->Buffer = Buffer.Gather(LocalIndirection);
	return Chunk;
}