// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBuffer.h"
#include "Sculpt/Height/VoxelHeightSculptDefinitions.h"

DECLARE_VOXEL_MEMORY_STAT(VOXEL_API, STAT_VoxelHeightMetadata_Memory, "Voxel Height Metadata Memory");

struct FVoxelHeightMetadataChunk
	: FVoxelHeightSculptDefinitions
	, TVoxelRefCountThis<FVoxelHeightMetadataChunk>
{
public:
	TVoxelStaticArray<uint8, ChunkCount> Alphas{ NoInit };
	TSharedPtr<FVoxelBuffer> Buffer;

	FVoxelHeightMetadataChunk();
	UE_NONCOPYABLE(FVoxelHeightMetadataChunk)

	VOXEL_COUNT_INSTANCES();
	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelHeightMetadata_Memory);

public:
	FORCEINLINE float GetAlpha(const int32 Index) const
	{
		return FVoxelUtilities::UINT8ToFloat(Alphas[Index]);
	}
	template<typename Type>
	FORCEINLINE Type GetValue(const int32 Index) const
	{
		checkVoxelSlow(Buffer.IsValid());
		return Buffer.Get()->AsChecked<TVoxelBufferType<Type>>()[Index];
	}

public:
	void Serialize(FArchive& Ar);
	int64 GetAllocatedSize() const;
	TVoxelRefCountPtr<FVoxelHeightMetadataChunk> Clone() const;
};