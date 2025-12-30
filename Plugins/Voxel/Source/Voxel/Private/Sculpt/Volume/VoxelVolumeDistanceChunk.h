// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelVolumeBlendMode.h"
#include "Sculpt/Volume/VoxelVolumeSculptDefinitions.h"

DECLARE_VOXEL_MEMORY_STAT(VOXEL_API, STAT_VoxelVolumeDistance_Memory, "Voxel Volume Distance Memory");

struct FVoxelVolumeDistanceChunk
	: FVoxelVolumeSculptDefinitions
	, TVoxelRefCountThis<FVoxelVolumeDistanceChunk>
{
public:
	TVoxelStaticArray<float, ChunkCount> AdditiveDistances{ NoInit };
	TVoxelStaticArray<float, ChunkCount> SubtractiveDistances{ NoInit };

	FVoxelVolumeDistanceChunk();
	UE_NONCOPYABLE(FVoxelVolumeDistanceChunk)

	VOXEL_COUNT_INSTANCES();
	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelVolumeDistance_Memory);

public:
	void Serialize(
		FArchive& Ar,
		int32 Version);

	int64 GetAllocatedSize() const;
	TVoxelRefCountPtr<FVoxelVolumeDistanceChunk> Clone() const;

public:
	static FORCEINLINE float Blend(
		const float OldDistance,
		const float AdditiveDistance,
		const float SubtractiveDistance,
		const EVoxelVolumeBlendMode BlendMode,
		const bool bApplyOnVoid)
	{
		switch (BlendMode)
		{
		default: VOXEL_ASSUME(false);
		case EVoxelVolumeBlendMode::Additive:
		{
			if (FVoxelUtilities::IsNaN(AdditiveDistance))
			{
				return OldDistance;
			}

			if (FVoxelUtilities::IsNaN(OldDistance))
			{
				if (bApplyOnVoid)
				{
					return AdditiveDistance;
				}
				else
				{
					return OldDistance;
				}
			}

			return FMath::Min(OldDistance, AdditiveDistance);
		}
		case EVoxelVolumeBlendMode::Subtractive:
		{
			if (FVoxelUtilities::IsNaN(SubtractiveDistance))
			{
				return OldDistance;
			}

			if (FVoxelUtilities::IsNaN(OldDistance))
			{
				if (bApplyOnVoid)
				{
					return SubtractiveDistance;
				}
				else
				{
					return OldDistance;
				}
			}

			return FMath::Max(OldDistance, SubtractiveDistance);
		}
		case EVoxelVolumeBlendMode::Intersect:
		{
			if (FVoxelUtilities::IsNaN(OldDistance))
			{
				return OldDistance;
			}
			if (FVoxelUtilities::IsNaN(AdditiveDistance))
			{
				return AdditiveDistance;
			}

			return FMath::Max(OldDistance, AdditiveDistance);
		}
		case EVoxelVolumeBlendMode::Override:
		{
			float Distance = OldDistance;
			if (!FVoxelUtilities::IsNaN(AdditiveDistance))
			{
				if (FVoxelUtilities::IsNaN(Distance))
				{
					Distance = AdditiveDistance;
				}
				else
				{
					Distance = FMath::Min(Distance, AdditiveDistance);
				}
			}
			if (!FVoxelUtilities::IsNaN(SubtractiveDistance))
			{
				if (FVoxelUtilities::IsNaN(Distance))
				{
					Distance = SubtractiveDistance;
				}
				else
				{
					Distance = FMath::Max(Distance, SubtractiveDistance);
				}
			}
			return Distance;
		}
		}
	}
};