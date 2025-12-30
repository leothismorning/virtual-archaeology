// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Sculpt/Height/VoxelHeightSculptDefinitions.h"

class FVoxelLayers;
class FVoxelSurfaceTypeTable;
struct FVoxelWeakStackLayer;

DECLARE_UNIQUE_VOXEL_ID(FVoxelHeightSculptDataId);

DECLARE_VOXEL_MEMORY_STAT(VOXEL_API, STAT_VoxelHeightSculptCache_Memory, "Voxel Height Sculpt Cache Chunk Memory");

struct FVoxelHeightSculptPreviousChunk : FVoxelHeightSculptDefinitions
{
public:
	TVoxelStaticArray<float, ChunkCount> Heights{ NoInit };

	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelHeightSculptCache_Memory);

	int64 GetAllocatedSize() const;

private:
	TSharedPtr<FVoxelDependencyTracker> DependencyTracker;

	FVoxelHeightSculptPreviousChunk();

	friend class FVoxelHeightSculptCache;
};

class FVoxelHeightSculptCache : FVoxelHeightSculptDefinitions
{
public:
	const FTransform2d SculptToWorld;
	const FVoxelHeightSculptDataId SculptDataId;

	FVoxelHeightSculptCache(
		const FTransform2d& SculptToWorld,
		FVoxelHeightSculptDataId SculptDataId);
	~FVoxelHeightSculptCache();

public:
	TSharedRef<const FVoxelHeightSculptPreviousChunk> ComputeChunk(
		const FVoxelLayers& Layers,
		const FVoxelSurfaceTypeTable& SurfaceTypeTable,
		const FVoxelWeakStackLayer& WeakLayer,
		const FIntPoint& ChunkKey);

private:
	FVoxelSharedCriticalSection CriticalSection;
	TVoxelMap<FIntPoint, TSharedPtr<const FVoxelHeightSculptPreviousChunk>> ChunkKeyToChunk_RequiresLock;
};