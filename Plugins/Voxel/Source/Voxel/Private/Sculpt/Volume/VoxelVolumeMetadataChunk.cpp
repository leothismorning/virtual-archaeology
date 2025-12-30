// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Volume/VoxelVolumeMetadataChunk.h"
#include "Sculpt/Volume/VoxelVolumeSculptVersion.h"

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelVolumeMetadataChunk);
DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelVolumeMetadata_Memory);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelVolumeMetadataChunk::FVoxelVolumeMetadataChunk()
{
	UpdateStats();
}

void FVoxelVolumeMetadataChunk::Serialize(
	FArchive& Ar,
	const int32 Version)
{
	VOXEL_FUNCTION_COUNTER();

	if (Version < FVoxelVolumeSculptVersion::MergeVersions)
	{
		using FVersion = DECLARE_VOXEL_VERSION
		(
			FirstVersion
		);

		int32 LegacyVersion = FVersion::LatestVersion;
		Ar << LegacyVersion;
	}

	Ar << Alphas;
	FVoxelBuffer::Serialize(Ar, Buffer);
}

int64 FVoxelVolumeMetadataChunk::GetAllocatedSize() const
{
	int64 AllocatedSize = 0;
	AllocatedSize += Alphas.GetAllocatedSize();
	if (Buffer)
	{
		AllocatedSize += Buffer->GetAllocatedSize();
	}
	return AllocatedSize;
}

TVoxelRefCountPtr<FVoxelVolumeMetadataChunk> FVoxelVolumeMetadataChunk::Clone() const
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelVolumeMetadataChunk* Result = new FVoxelVolumeMetadataChunk();
	Result->Alphas = Alphas;
	if (ensure(Buffer))
	{
		Result->Buffer = Buffer->MakeDeepCopy();
	}

	Result->UpdateStats();
	return Result;
}