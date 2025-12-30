// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.h"
#include "Interfaces/IPluginManager.h"

VOXEL_DEFAULT_MODULE(Voxel);

#if WITH_EDITOR
VOXEL_RUN_ON_STARTUP_GAME()
{
	if (FVoxelUtilities::IsDevWorkflow() &&
		GIsEditor)
	{
		FVoxelUtilities::CleanupRedirects(FVoxelUtilities::GetPlugin().GetBaseDir() / "Config" / "BaseVoxel.ini");
	}
}
#endif