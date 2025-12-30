// Copyright Voxel Plugin SAS. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class Voxel : ModuleRules
{
	public Voxel(ReadOnlyTargetRules Target) : base(Target)
	{
		SetupVoxelModule(this);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"VoxelGraph",
			"Chaos",
			"Renderer",
			"Slate",
			"SlateCore",
			"PhysicsCore",
			"Landscape",
			"NavigationSystem",
			"PCG",
			"MeshDescription",
			"StaticMeshDescription",
		});

		PrivateIncludePaths.AddRange(new string[]
		{
			Path.Combine(EngineDirectory, "Source/Runtime/Engine/Private/"),
			Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Private"),
			Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Internal")
		});
	}
}