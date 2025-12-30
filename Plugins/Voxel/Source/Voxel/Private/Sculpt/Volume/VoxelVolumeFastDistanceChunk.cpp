// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Volume/VoxelVolumeFastDistanceChunk.h"
#include "Sculpt/Volume/VoxelVolumeSculptVersion.h"

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelVolumeFastDistanceChunk);
DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelVolumeFastDistance_Memory);

FVoxelVolumeFastDistanceChunk::FVoxelVolumeFastDistanceChunk()
{
#if VOXEL_DEBUG
	Distances.Memset(0xDE);
#endif

	UpdateStats();
}

void FVoxelVolumeFastDistanceChunk::Serialize(
	FArchive& Ar,
	const int32 Version)
{
	VOXEL_FUNCTION_COUNTER();

	Ar << Distances;
}

int64 FVoxelVolumeFastDistanceChunk::GetAllocatedSize() const
{
	return Distances.GetAllocatedSize();
}

TVoxelRefCountPtr<FVoxelVolumeFastDistanceChunk> FVoxelVolumeFastDistanceChunk::Clone() const
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelVolumeFastDistanceChunk* Result = new FVoxelVolumeFastDistanceChunk();
	Result->Distances = Distances;
	return Result;
}