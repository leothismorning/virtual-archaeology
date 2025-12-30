// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Height/VoxelHeightMetadataChunk.h"

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelHeightMetadataChunk);
DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelHeightMetadata_Memory);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelHeightMetadataChunk::FVoxelHeightMetadataChunk()
{
	UpdateStats();
}

void FVoxelHeightMetadataChunk::Serialize(FArchive& Ar)
{
	VOXEL_FUNCTION_COUNTER();

	using FVersion = DECLARE_VOXEL_VERSION
	(
		FirstVersion
	);

	int32 Version = FVersion::LatestVersion;
	Ar << Version;
	Ar << Alphas;
	FVoxelBuffer::Serialize(Ar, Buffer);
}

int64 FVoxelHeightMetadataChunk::GetAllocatedSize() const
{
	int64 AllocatedSize = 0;
	AllocatedSize += Alphas.GetAllocatedSize();
	if (Buffer)
	{
		AllocatedSize += Buffer->GetAllocatedSize();
	}
	return AllocatedSize;
}

TVoxelRefCountPtr<FVoxelHeightMetadataChunk> FVoxelHeightMetadataChunk::Clone() const
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelHeightMetadataChunk* Result = new FVoxelHeightMetadataChunk();
	Result->Alphas = Alphas;
	if (ensure(Buffer))
	{
		Result->Buffer = Buffer->MakeDeepCopy();
	}

	Result->UpdateStats();
	return Result;
}