// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelHeightBlendMode.h"
#include "Sculpt/Height/VoxelHeightSculptDefinitions.h"

DECLARE_VOXEL_MEMORY_STAT(VOXEL_API, STAT_VoxelHeightHeight_Memory, "Voxel Height Height Memory");

struct FVoxelHeightHeightChunk
	: FVoxelHeightSculptDefinitions
	, TVoxelRefCountThis<FVoxelHeightHeightChunk>
{
public:
	TVoxelStaticArray<float, ChunkCount> Heights{ NoInit };

	FVoxelHeightHeightChunk();
	UE_NONCOPYABLE(FVoxelHeightHeightChunk)

	VOXEL_COUNT_INSTANCES();
	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelHeightHeight_Memory);

public:
	void Serialize(FArchive& Ar);
	int64 GetAllocatedSize() const;
	TVoxelRefCountPtr<FVoxelHeightHeightChunk> Clone() const;

public:
	static FORCEINLINE float Blend(
		const float OldHeight,
		const float NewHeight,
		const EVoxelHeightBlendMode BlendMode,
		const bool bApplyOnVoid)
	{
		if (FVoxelUtilities::IsNaN(NewHeight))
		{
			return OldHeight;
		}

		switch (BlendMode)
		{
		default: VOXEL_ASSUME(false);
		case EVoxelHeightBlendMode::Min:
		{
			if (FVoxelUtilities::IsNaN(OldHeight))
			{
				if (bApplyOnVoid)
				{
					return NewHeight;
				}
				else
				{
					return OldHeight;
				}
			}

			return FMath::Min(OldHeight, NewHeight);
		}
		case EVoxelHeightBlendMode::Max:
		{
			if (FVoxelUtilities::IsNaN(OldHeight))
			{
				if (bApplyOnVoid)
				{
					return NewHeight;
				}
				else
				{
					return OldHeight;
				}
			}

			return FMath::Max(OldHeight, NewHeight);
		}
		case EVoxelHeightBlendMode::Override:
		{
			return NewHeight;
		}
		}
	}
};