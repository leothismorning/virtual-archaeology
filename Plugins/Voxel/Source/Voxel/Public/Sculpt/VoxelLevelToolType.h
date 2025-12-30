// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLevelToolType.generated.h"

UENUM(BlueprintType, meta = (VoxelSegmentedEnum))
enum class EVoxelLevelToolType : uint8
{
	Additive UMETA(StyleSet = "VoxelStyle", Icon = "Voxel.Sculpt.LevelTool.Additive"),
	Subtractive UMETA(StyleSet = "VoxelStyle", Icon = "Voxel.Sculpt.LevelTool.Subtractive"),
	Both UMETA(StyleSet = "VoxelStyle", Icon = "Voxel.Sculpt.LevelTool.Both")
};