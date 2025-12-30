// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Height/VoxelHeightHeightChunk.h"

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelHeightHeightChunk);
DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelHeightHeight_Memory);

FVoxelHeightHeightChunk::FVoxelHeightHeightChunk()
{
#if VOXEL_DEBUG
	Heights.Memset(0xDE);
#endif

	UpdateStats();
}

void FVoxelHeightHeightChunk::Serialize(FArchive& Ar)
{
	VOXEL_FUNCTION_COUNTER();

	using FVersion = DECLARE_VOXEL_VERSION
	(
		FirstVersion
	);

	int32 Version = FVersion::LatestVersion;
	Ar << Version;

	Ar << Heights;
}

int64 FVoxelHeightHeightChunk::GetAllocatedSize() const
{
	int64 AllocatedSize = 0;
	AllocatedSize += Heights.GetAllocatedSize();
	return AllocatedSize;
}

TVoxelRefCountPtr<FVoxelHeightHeightChunk> FVoxelHeightHeightChunk::Clone() const
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelHeightHeightChunk* Result = new FVoxelHeightHeightChunk();
	Result->Heights = Heights;

	Result->UpdateStats();
	return Result;
}