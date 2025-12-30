// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMetadataRef.h"
#include "VoxelNormalMetadataRef.generated.h"

USTRUCT()
struct VOXEL_API FVoxelNormalMetadataRef : public FVoxelMetadataRef
{
	GENERATED_BODY()
	GENERATED_VOXEL_METADATA_REF_BODY(FVoxelNormalMetadataRef, UVoxelNormalMetadata, FVoxelNormalBuffer);
};