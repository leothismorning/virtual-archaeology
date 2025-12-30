// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelTerminalBuffer.h"
#include "VoxelClassBuffer.generated.h"

DECLARE_VOXEL_TERMINAL_BUFFER(FVoxelClassBuffer, TSubclassOf<UObject>);

USTRUCT(DisplayName = "Class Buffer")
struct VOXELGRAPH_API FVoxelClassBuffer final : public FVoxelTerminalBuffer
{
	GENERATED_BODY()
	GENERATED_VOXEL_TERMINAL_BUFFER_BODY(FVoxelClassBuffer, TSubclassOf<UObject>);
};