// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMetadataRef.h"
#include "VoxelFloatMetadataRef.generated.h"

USTRUCT()
struct VOXEL_API FVoxelFloatMetadataRef : public FVoxelMetadataRef
{
	GENERATED_BODY()
	GENERATED_VOXEL_METADATA_REF_BODY(FVoxelFloatMetadataRef, UVoxelFloatMetadata, FVoxelFloatBuffer);
};