// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Volume/VoxelVolumeModifier.h"

void FVoxelVolumeModifier::Initialize_GameThread()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	ensure(!bInitialized);
	bInitialized = true;
}