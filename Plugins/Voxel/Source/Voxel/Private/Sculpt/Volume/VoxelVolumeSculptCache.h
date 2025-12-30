// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Sculpt/Volume/VoxelVolumeSculptDefinitions.h"

class FVoxelLayers;
class FVoxelSurfaceTypeTable;
struct FVoxelWeakStackLayer;

DECLARE_UNIQUE_VOXEL_ID(FVoxelVolumeSculptDataId);

DECLARE_VOXEL_MEMORY_STAT(VOXEL_API, STAT_VoxelVolumeSculptCache_Memory, "Voxel Volume Sculpt Cache Chunk Memory");

struct FVoxelVolumeSculptPreviousChunk : FVoxelVolumeSculptDefinitions
{
public:
	TVoxelStaticArray<float, ChunkCount> Distances{ NoInit };

	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelVolumeSculptCache_Memory);

	int64 GetAllocatedSize() const;

private:
	TSharedPtr<FVoxelDependencyTracker> DependencyTracker;

	FVoxelVolumeSculptPreviousChunk();

	friend class FVoxelVolumeSculptCache;
};

class FVoxelVolumeSculptCache : FVoxelVolumeSculptDefinitions
{
public:
	const FMatrix SculptToWorld;
	const FVoxelVolumeSculptDataId SculptDataId;

	FVoxelVolumeSculptCache(
		const FMatrix& SculptToWorld,
		FVoxelVolumeSculptDataId SculptDataId);
	~FVoxelVolumeSculptCache();

public:
	TSharedRef<const FVoxelVolumeSculptPreviousChunk> ComputeChunk(
		const FVoxelLayers& Layers,
		const FVoxelSurfaceTypeTable& SurfaceTypeTable,
		const FVoxelWeakStackLayer& WeakLayer,
		const FIntVector& ChunkKey);

private:
	FVoxelSharedCriticalSection CriticalSection;
	TVoxelMap<FIntVector, TSharedPtr<const FVoxelVolumeSculptPreviousChunk>> ChunkKeyToChunk_RequiresLock;
};