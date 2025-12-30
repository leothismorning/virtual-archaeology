// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "Nodes/VoxelOutputNode.h"
#include "VoxelOutputNode_Preview.generated.h"

USTRUCT(meta = (Internal))
struct VOXELGRAPH_API FVoxelOutputNode_Preview : public FVoxelOutputNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelWildcard, Value, nullptr);
};