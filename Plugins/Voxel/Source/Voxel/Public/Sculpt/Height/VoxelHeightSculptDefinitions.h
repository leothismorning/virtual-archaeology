// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

struct FVoxelHeightSculptDefinitions
{
	static constexpr int32 ChunkSizeLog2 = 4;
	static constexpr int32 ChunkSize = 1 << ChunkSizeLog2;
	static constexpr int32 ChunkCount = FMath::Square(ChunkSize);
};