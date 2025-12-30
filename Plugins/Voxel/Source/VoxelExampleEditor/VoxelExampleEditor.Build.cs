// Copyright Voxel Plugin SAS. All Rights Reserved.

using UnrealBuildTool;

public class VoxelExampleEditor : ModuleRules
{
    public VoxelExampleEditor(ReadOnlyTargetRules Target) : base(Target)
    {
	    SetupVoxelModule(this);

	    PublicDependencyModuleNames.AddRange(new string[]
	    {
	    });
    }
}