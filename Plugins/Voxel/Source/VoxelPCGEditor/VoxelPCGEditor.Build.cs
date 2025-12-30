// Copyright Voxel Plugin SAS. All Rights Reserved.

using UnrealBuildTool;

public class VoxelPCGEditor : ModuleRules
{
	public VoxelPCGEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		SetupVoxelModule(this);

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"VoxelPCG",
			"VoxelGraph",
			"VoxelGraphEditor",
			"PCG",
			"PCGEditor",
		});
	}
}