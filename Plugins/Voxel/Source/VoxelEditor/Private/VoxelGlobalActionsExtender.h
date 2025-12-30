// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"

struct FToolMenuEntry;

class FVoxelGlobalActionsExtender
{
public:
	static void RegisterMenu();
	static void UnregisterMenu();
};