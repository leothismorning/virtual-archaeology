// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelNode_CustomizeParameter.generated.h"

USTRUCT(meta = (Internal))
struct VOXELGRAPH_API FVoxelNode_CustomizeParameter : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	UPROPERTY()
	FGuid ParameterGuid;

	UPROPERTY()
	FName ParameterName;

	VOXEL_INPUT_PIN(bool, IsVisible, true);
	VOXEL_INPUT_PIN(bool, IsReadOnly, false);
	VOXEL_INPUT_PIN(FName, DisplayName, nullptr);
};