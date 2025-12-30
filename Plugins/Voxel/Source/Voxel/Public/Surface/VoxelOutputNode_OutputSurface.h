// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelSurfaceTypeBlendBuffer.h"
#include "Nodes/VoxelOutputNode.h"
#include "VoxelOutputNode_OutputSurface.generated.h"

USTRUCT()
struct VOXEL_API FVoxelOutputNode_OutputSurface : public FVoxelOutputNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelSurfaceTypeBlendBuffer, SurfaceType, nullptr);
};