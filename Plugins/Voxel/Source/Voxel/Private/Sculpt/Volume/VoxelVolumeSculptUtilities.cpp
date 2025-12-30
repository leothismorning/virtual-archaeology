// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Volume/VoxelVolumeSculptUtilities.h"
#include "Sculpt/Volume/VoxelVolumeDistanceChunk.h"
#include "Sculpt/Volume/VoxelVolumeMetadataChunk.h"
#include "Sculpt/Volume/VoxelVolumeSurfaceTypeChunk.h"
#include "Sculpt/Volume/VoxelVolumeFastDistanceChunk.h"
#include "Buffer/VoxelBaseBuffers.h"

void FVoxelVolumeSculptUtilities::DiffDistances(
	const TConstVoxelArrayView<float> Distances,
	const TConstVoxelArrayView<float> PreviousDistances,
	const FIntVector& Size,
	const float VoxelSize,
	TVoxelArray<float>& AdditiveDistances,
	TVoxelArray<float>& SubtractiveDistances)
{
	VOXEL_FUNCTION_COUNTER_NUM(Distances.Num());
	check(Distances.Num() == Size.X * Size.Y * Size.Z);
	check(PreviousDistances.Num() == Size.X * Size.Y * Size.Z);

	FVoxelUtilities::SetNumFast(AdditiveDistances, Distances.Num());
	FVoxelUtilities::SetNumFast(SubtractiveDistances, Distances.Num());

	Voxel::ParallelFor(Distances.Num(), [&](const int32 Index)
	{
		const float NewDistance = Distances[Index];
		const float PreviousDistance = PreviousDistances[Index];

		if (FVoxelUtilities::IsNaN(PreviousDistance))
		{
			AdditiveDistances[Index] = NewDistance;
			SubtractiveDistances[Index] = -NewDistance;
			return;
		}

		// FVoxelUtilities::JumpFlood considers 0 to be positive
		// To avoid having a fully positive distance field when making small surface edits,
		// force set values to be negative where the distance changed (where it decreased for additive, increased for subtractive)
		// This will be fixed up by maxing with NewDistance again
		// If we don't do this, JumpFlood will return NaNs everywhere
		// We don't want to do -VoxelSize everywhere though, as otherwise any naturally-occuring 0 in the original
		// surface will be added to the additive or subtractive distance fields
		// An easy test for that is sculpting a sphere on a plane at Z = 0: once the plane is removed, the sphere
		// distance field should be contained within the sphere and not "leak" at Z = 0
		// We invert SubtractiveDistance to ensure the behavior is the same sign-wise as additive distances,
		// as otherwise all the 0s would create unwanted surfaces

		AdditiveDistances[Index] = FMath::Max(NewDistance, -PreviousDistance) - VoxelSize * (NewDistance < PreviousDistance);
		SubtractiveDistances[Index] = FMath::Max(-NewDistance, PreviousDistance) - VoxelSize * (NewDistance > PreviousDistance);
	});

	FVoxelUtilities::JumpFlood(Size, VoxelSize, AdditiveDistances);
	FVoxelUtilities::JumpFlood(Size, VoxelSize, SubtractiveDistances);

	// Grow the distance fields to ensure we don't get glitches when interpolating between cells
	// eg, Voxel World with a Voxel Size of 10cm but sculpt volume with a Voxel Size of 100cm

	Voxel::ParallelFor(AdditiveDistances, [&](float& Value, const int32 Index)
	{
		if (FVoxelUtilities::IsNaN(Value))
		{
			return;
		}

		ensureVoxelSlow(!FVoxelUtilities::IsNaN(Distances[Index]));
		Value = FMath::Max(Value - VoxelSize, Distances[Index]);
	});

	Voxel::ParallelFor(SubtractiveDistances, [&](float& Value, const int32 Index)
	{
		if (FVoxelUtilities::IsNaN(Value))
		{
			return;
		}

		ensureVoxelSlow(!FVoxelUtilities::IsNaN(Distances[Index]));
		Value = FMath::Max(Value - VoxelSize, -Distances[Index]);
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelRefCountPtr<FVoxelVolumeFastDistanceChunk> FVoxelVolumeSculptUtilities::CreateFastDistanceChunk(
	const FIntVector& ChunkKey,
	const TConstVoxelArrayView<float> Distances,
	const FIntVector& Size,
	const float VoxelSize)
{
	VOXEL_FUNCTION_COUNTER();
	check(Distances.Num() == Size.X * Size.Y * Size.Z);

	TVoxelRefCountPtr<FVoxelVolumeFastDistanceChunk> Chunk = new FVoxelVolumeFastDistanceChunk();

	TVoxelStaticArray<float, ChunkCount> LocalDistances{ NoInit };

	float MinDistance = MAX_flt;
	float MaxDistance = -MAX_flt;
	for (int32 IndexZ = 0; IndexZ < ChunkSize; IndexZ++)
	{
		for (int32 IndexY = 0; IndexY < ChunkSize; IndexY++)
		{
			for (int32 IndexX = 0; IndexX < ChunkSize; IndexX++)
			{
				const int32 IndexInChunk = FVoxelUtilities::Get3DIndex<int32>(
					ChunkSize,
					IndexX,
					IndexY,
					IndexZ);

				const int32 IndexInSculpt = FVoxelUtilities::Get3DIndex<int32>(
					Size,
					ChunkKey.X * ChunkSize + IndexX,
					ChunkKey.Y * ChunkSize + IndexY,
					ChunkKey.Z * ChunkSize + IndexZ);

				float Distance = Distances[IndexInSculpt];
				ensureVoxelSlow(!FVoxelUtilities::IsNaN(Distance));
				Distance /= VoxelSize;

				MinDistance = FMath::Min(MinDistance, Distance);
				MaxDistance = FMath::Max(MaxDistance, Distance);

				LocalDistances[IndexInChunk] = Distance;
			}
		}
	}

	const bool bCheckBounds = MinDistance <= 0 && 0 <= MaxDistance;

	for (int32 Index = 0; Index < ChunkCount; Index++)
	{
		Chunk->Distances[Index] = FVoxelVolumeFastDistanceChunk::Pack(LocalDistances[Index], bCheckBounds);
	}

	return Chunk;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelRefCountPtr<FVoxelVolumeDistanceChunk> FVoxelVolumeSculptUtilities::CreateDistanceChunk_NoDiffing(
	const FIntVector& ChunkKey,
	const TConstVoxelArrayView<float> Distances,
	const FIntVector& Size,
	const float VoxelSize)
{
	VOXEL_FUNCTION_COUNTER();
	check(Distances.Num() == Size.X * Size.Y * Size.Z);

	TVoxelRefCountPtr<FVoxelVolumeDistanceChunk> Chunk = new FVoxelVolumeDistanceChunk();

	for (int32 IndexZ = 0; IndexZ < ChunkSize; IndexZ++)
	{
		for (int32 IndexY = 0; IndexY < ChunkSize; IndexY++)
		{
			for (int32 IndexX = 0; IndexX < ChunkSize; IndexX++)
			{
				const int32 IndexInChunk = FVoxelUtilities::Get3DIndex<int32>(
					ChunkSize,
					IndexX,
					IndexY,
					IndexZ);

				const int32 IndexInSculpt = FVoxelUtilities::Get3DIndex<int32>(
					Size,
					ChunkKey.X * ChunkSize + IndexX,
					ChunkKey.Y * ChunkSize + IndexY,
					ChunkKey.Z * ChunkSize + IndexZ);

				float Distance = Distances[IndexInSculpt];
				ensureVoxelSlow(!FVoxelUtilities::IsNaN(Distance));
				Distance /= VoxelSize;

				Chunk->AdditiveDistances[IndexInChunk] = Distance;
				Chunk->SubtractiveDistances[IndexInChunk] = -Distance;
			}
		}
	}

	return Chunk;
}

TVoxelRefCountPtr<FVoxelVolumeDistanceChunk> FVoxelVolumeSculptUtilities::CreateDistanceChunk_Diffing(
	const FIntVector& ChunkKey,
	const TConstVoxelArrayView<float> AdditiveDistances,
	const TConstVoxelArrayView<float> SubtractiveDistances,
	const FIntVector& Size,
	const float VoxelSize)
{
	VOXEL_FUNCTION_COUNTER();
	check(AdditiveDistances.Num() == Size.X * Size.Y * Size.Z);
	check(SubtractiveDistances.Num() == Size.X * Size.Y * Size.Z);

	TVoxelRefCountPtr<FVoxelVolumeDistanceChunk> Chunk = new FVoxelVolumeDistanceChunk();

	for (int32 IndexZ = 0; IndexZ < ChunkSize; IndexZ++)
	{
		for (int32 IndexY = 0; IndexY < ChunkSize; IndexY++)
		{
			for (int32 IndexX = 0; IndexX < ChunkSize; IndexX++)
			{
				const int32 IndexInChunk = FVoxelUtilities::Get3DIndex<int32>(
					ChunkSize,
					IndexX,
					IndexY,
					IndexZ);

				const int32 IndexInSculpt = FVoxelUtilities::Get3DIndex<int32>(
					Size,
					ChunkKey.X * ChunkSize + IndexX,
					ChunkKey.Y * ChunkSize + IndexY,
					ChunkKey.Z * ChunkSize + IndexZ);

				float AdditiveDistance = AdditiveDistances[IndexInSculpt];
				float SubtractiveDistance = SubtractiveDistances[IndexInSculpt];

				if (!FVoxelUtilities::IsNaN(AdditiveDistance))
				{
					AdditiveDistance /= VoxelSize;
				}
				if (!FVoxelUtilities::IsNaN(SubtractiveDistance))
				{
					SubtractiveDistance /= VoxelSize;
				}

				Chunk->AdditiveDistances[IndexInChunk] = AdditiveDistance;
				Chunk->SubtractiveDistances[IndexInChunk] = SubtractiveDistance;
			}
		}
	}

	return Chunk;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelRefCountPtr<FVoxelVolumeSurfaceTypeChunk> FVoxelVolumeSculptUtilities::CreateSurfaceTypeChunk(
	const FIntVector& ChunkKey,
	const TVoxelOptional<TConstVoxelArrayView<int32>>& Indirection,
	const TConstVoxelArrayView<float> Alphas,
	const TConstVoxelArrayView<FVoxelSurfaceTypeBlend> SurfaceTypes,
	const FIntVector& Size)
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelRefCountPtr<FVoxelVolumeSurfaceTypeChunk> Chunk;

	{
		VOXEL_SCOPE_COUNTER("UsedSurfaceTypes");

		int32 NumLayers = 0;

		TVoxelArray<FVoxelSurfaceType> UsedSurfaceTypes;
		UsedSurfaceTypes.Reserve(16);

		for (int32 IndexZ = 0; IndexZ < ChunkSize; IndexZ++)
		{
			for (int32 IndexY = 0; IndexY < ChunkSize; IndexY++)
			{
				for (int32 IndexX = 0; IndexX < ChunkSize; IndexX++)
				{
					int32 Index = FVoxelUtilities::Get3DIndex<int32>(
						Size,
						ChunkKey.X * ChunkSize + IndexX,
						ChunkKey.Y * ChunkSize + IndexY,
						ChunkKey.Z * ChunkSize + IndexZ);

					if (Indirection)
					{
						Index = (*Indirection)[Index];

						if (Index == -1)
						{
							continue;
						}
					}

					const FVoxelSurfaceTypeBlend& SurfaceType = SurfaceTypes[Index];

					NumLayers = FMath::Max(NumLayers, SurfaceType.GetLayers().Num());

					for (const FVoxelSurfaceTypeBlendLayer& Layer : SurfaceType.GetLayers())
					{
						UsedSurfaceTypes.AddUnique(Layer.Type);
					}
				}
			}
		}

		if (NumLayers == 0)
		{
			return nullptr;
		}

		Chunk = new FVoxelVolumeSurfaceTypeChunk();
		Chunk->Layers.SetNum(NumLayers);
		Chunk->UsedSurfaceTypes = MoveTemp(UsedSurfaceTypes);
	}

	FVoxelUtilities::Memzero_Stats(Chunk->Layers);

	for (int32 IndexZ = 0; IndexZ < ChunkSize; IndexZ++)
	{
		for (int32 IndexY = 0; IndexY < ChunkSize; IndexY++)
		{
			for (int32 IndexX = 0; IndexX < ChunkSize; IndexX++)
			{
				int32 Index = FVoxelUtilities::Get3DIndex<int32>(
					Size,
					ChunkKey.X * ChunkSize + IndexX,
					ChunkKey.Y * ChunkSize + IndexY,
					ChunkKey.Z * ChunkSize + IndexZ);

				if (Indirection)
				{
					Index = (*Indirection)[Index];

					if (Index == -1)
					{
						continue;
					}
				}

				const int32 IndexInChunk = FVoxelUtilities::Get3DIndex<int32>(
					ChunkSize,
					IndexX,
					IndexY,
					IndexZ);

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
					FVoxelVolumeSurfaceTypeChunk::FLayer& ChunkLayer = Chunk->Layers[LayerIndex++];

					const int32 TypeIndex = Chunk->UsedSurfaceTypes.Find(Layer.Type);
					checkVoxelSlow(FVoxelUtilities::IsValidUINT8(TypeIndex));

					ChunkLayer.Types[IndexInChunk] = uint8(TypeIndex);
					ChunkLayer.Weights[IndexInChunk] = FVoxelUtilities::FloatToUINT8(Layer.Weight / MaxWeight);
				}
			}
		}
	}

	return Chunk;
}

TVoxelRefCountPtr<FVoxelVolumeMetadataChunk> FVoxelVolumeSculptUtilities::CreateMetadataChunk(
	const FIntVector& ChunkKey,
	const TVoxelOptional<TConstVoxelArrayView<int32>>& Indirection,
	const TConstVoxelArrayView<float> Alphas,
	const FVoxelBuffer& Buffer,
	const FIntVector& Size)
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelRefCountPtr<FVoxelVolumeMetadataChunk> Chunk = new FVoxelVolumeMetadataChunk();

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
					const int32 IndexInChunk = FVoxelUtilities::Get3DIndex<int32>(
						ChunkSize,
						IndexX,
						IndexY,
						IndexZ);

					int32 Index = FVoxelUtilities::Get3DIndex<int32>(
						Size,
						ChunkKey.X * ChunkSize + IndexX,
						ChunkKey.Y * ChunkSize + IndexY,
						ChunkKey.Z * ChunkSize + IndexZ);

					if (Indirection)
					{
						Index = (*Indirection)[Index];
					}

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