// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelTerminalBuffer.h"
#include "VoxelNormalBuffer.generated.h"

DECLARE_VOXEL_TERMINAL_BUFFER(FVoxelNormalBuffer, FVoxelOctahedron);

USTRUCT(DisplayName = "Normal Buffer")
struct VOXELGRAPH_API FVoxelNormalBuffer final : public FVoxelTerminalBuffer
{
	GENERATED_BODY()
	GENERATED_VOXEL_TERMINAL_BUFFER_BODY(FVoxelNormalBuffer, FVoxelOctahedron);
};