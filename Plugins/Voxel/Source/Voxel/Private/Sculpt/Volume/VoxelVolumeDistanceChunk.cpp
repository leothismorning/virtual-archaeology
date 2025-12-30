// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Volume/VoxelVolumeDistanceChunk.h"
#include "Sculpt/Volume/VoxelVolumeSculptVersion.h"

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelVolumeDistanceChunk);
DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelVolumeDistance_Memory);

FVoxelVolumeDistanceChunk::FVoxelVolumeDistanceChunk()
{
#if VOXEL_DEBUG
	AdditiveDistances.Memset(0xDE);
	SubtractiveDistances.Memset(0xDE);
#endif

	UpdateStats();
}

void FVoxelVolumeDistanceChunk::Serialize(
	FArchive& Ar,
	const int32 Version)
{
	VOXEL_FUNCTION_COUNTER();

	using FVersion = DECLARE_VOXEL_VERSION
	(
		FirstVersion,
		InvertSubtractiveDistances,
		OptionalDiffing,
		RemoveOptionalDiffing
	);

	int32 LegacyVersion = FVersion::LatestVersion;

	if (Version < FVoxelVolumeSculptVersion::MergeVersions)
	{
		Ar << LegacyVersion;

		if (LegacyVersion >= FVersion::OptionalDiffing &&
			LegacyVersion < FVersion::RemoveOptionalDiffing)
		{
			bool bEnableDiffing = true;
			Ar << bEnableDiffing;

			if (!ensure(bEnableDiffing))
			{
				return;
			}
		}
	}

	Ar << AdditiveDistances;
	Ar << SubtractiveDistances;

	if (LegacyVersion < FVersion::InvertSubtractiveDistances)
	{
		for (float& Distance : SubtractiveDistances)
		{
			if (!FVoxelUtilities::IsNaN(Distance))
			{
				Distance = -Distance;
			}
		}
	}
}

int64 FVoxelVolumeDistanceChunk::GetAllocatedSize() const
{
	int64 AllocatedSize = 0;
	AllocatedSize += AdditiveDistances.GetAllocatedSize();
	AllocatedSize += SubtractiveDistances.GetAllocatedSize();
	return AllocatedSize;
}

TVoxelRefCountPtr<FVoxelVolumeDistanceChunk> FVoxelVolumeDistanceChunk::Clone() const
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelVolumeDistanceChunk* Result = new FVoxelVolumeDistanceChunk();
	Result->AdditiveDistances = AdditiveDistances;
	Result->SubtractiveDistances = SubtractiveDistances;

	Result->UpdateStats();
	return Result;
}