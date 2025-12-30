// Copyright Voxel Plugin SAS. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class VoxelCoreEditor : ModuleRules
{
    public VoxelCoreEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		SetupVoxelModule(this);

		IWYUSupport = IWYUSupport.None;

		// For FDetailCategoryBuilderImpl
		PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Source/Editor/PropertyEditor/Private"));

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"AdvancedPreviewScene"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"PlacementMode",
			"MessageLog",
			"WorkspaceMenuStructure",
			"DetailCustomizations",
			"BlueprintGraph",
			"Landscape",
			"ToolMenus",
			"GraphEditor",
			"SceneOutliner",
			"SettingsEditor",
			"ApplicationCore",
			"SharedSettingsWidgets",
			"UMG",
		});
    }
}