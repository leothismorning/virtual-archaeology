// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBuffer.h"
#include "Sculpt/Volume/VoxelVolumeSculptDefinitions.h"

DECLARE_VOXEL_MEMORY_STAT(VOXEL_API, STAT_VoxelVolumeMetadata_Memory, "Voxel Volume Metadata Memory");

struct FVoxelVolumeMetadataChunk
	: FVoxelVolumeSculptDefinitions
	, TVoxelRefCountThis<FVoxelVolumeMetadataChunk>
{
public:
	TVoxelStaticArray<uint8, ChunkCount> Alphas{ NoInit };
	TSharedPtr<FVoxelBuffer> Buffer;

	FVoxelVolumeMetadataChunk();
	UE_NONCOPYABLE(FVoxelVolumeMetadataChunk)

	VOXEL_COUNT_INSTANCES();
	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelVolumeMetadata_Memory);

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
	void Serialize(
		FArchive& Ar,
		int32 Version);

	int64 GetAllocatedSize() const;
	TVoxelRefCountPtr<FVoxelVolumeMetadataChunk> Clone() const;
};