// Copyright Voxel Plugin SAS. All Rights Reserved.

using UnrealBuildTool;

public class VoxelPCG : ModuleRules
{
    public VoxelPCG(ReadOnlyTargetRules Target) : base(Target)
	{
		SetupVoxelModule(this);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Voxel",
			"VoxelGraph",
			"PCG",
			"Slate",
			"SlateCore",
		});
	}
}