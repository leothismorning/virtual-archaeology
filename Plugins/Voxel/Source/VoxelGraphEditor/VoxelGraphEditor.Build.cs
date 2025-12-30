// Copyright Voxel Plugin SAS. All Rights Reserved.

using UnrealBuildTool;

public class VoxelGraphEditor : ModuleRules
{
    public VoxelGraphEditor(ReadOnlyTargetRules Target) : base(Target)
    {
	    SetupVoxelModule(this);

	    PublicDependencyModuleNames.AddRange(new string[]
	    {
		    "VoxelGraph"
	    });

	    PrivateDependencyModuleNames.AddRange(new string[]
	    {
		    "Voxel",
		    "VoxelPCG",
		    "UMG",
		    "HTTP",
		    "MainFrame",
		    "ToolMenus",
		    "MessageLog",
		    "CurveEditor",
		    "GraphEditor",
		    "KismetWidgets",
		    "EditorWidgets",
		    "BlueprintGraph",
		    "ApplicationCore",
		    "SlateRHIRenderer",
		    "SharedSettingsWidgets",
		    "WorkspaceMenuStructure",
		    "InteractiveToolsFramework",
		    "EditorInteractiveToolsFramework",
	    });
    }
}