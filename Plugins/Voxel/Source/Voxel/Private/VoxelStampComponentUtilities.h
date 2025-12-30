// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

struct FVoxelStampRuntime;

struct FVoxelStampComponentUtilities
{
	static bool ShouldRender(const USceneComponent* Component);
	static FVoxelBox GetLocalBounds(const FVoxelStampRuntime& Stamp);
};