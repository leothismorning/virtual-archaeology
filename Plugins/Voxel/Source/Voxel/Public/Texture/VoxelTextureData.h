// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

class UVoxelTexture;
struct FVoxelBuffer;

DECLARE_VOXEL_MEMORY_STAT(VOXEL_API, STAT_VoxelTextureMemory, "Voxel Texture Memory");

enum class EVoxelTextureDataType : uint8
{
	Byte,
	Color,
	LinearColor,
	UINT16,
	Float
};
VOXEL_API EVoxelTextureDataType GetVoxelTextureDataType(const FVoxelBuffer& Buffer);

class VOXEL_API FVoxelTextureData
{
public:
	int32 SizeX = 0;
	int32 SizeY = 0;
	TSharedPtr<FVoxelBuffer> Buffer;
	EVoxelTextureDataType Type = {};

	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelTextureMemory);

public:
	float Sample(
		const FVector2D& Position,
		EVoxelTextureChannel Channel) const;

public:
#if WITH_EDITOR
	TSoftObjectPtr<UTexture2D> Texture;

	static TSharedPtr<const FVoxelTextureData> Create(const UVoxelTexture& VoxelTexture);
#endif

public:
	void Serialize(FArchive& Ar, UObject* Owner);
	int64 GetAllocatedSize() const;

private:
#if WITH_EDITOR
	static TSharedPtr<FVoxelBuffer> CreateImpl(
		const UVoxelTexture& VoxelTexture,
		int32& OutSizeX,
		int32& OutSizeY);
#endif
};