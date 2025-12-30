// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelStampComponent.h"

VOXEL_CUSTOMIZE_CLASS(UVoxelStampComponent)(IDetailLayoutBuilder& DetailLayout)
{
	FVoxelEditorUtilities::EnableRealtime();

	DetailLayout.HideCategory("Activation");
	DetailLayout.HideCategory("Rendering");
	DetailLayout.HideCategory("Cooking");
	DetailLayout.HideCategory("Physics");
	DetailLayout.HideCategory("LOD");
	DetailLayout.HideCategory("AssetUserData");
	DetailLayout.HideCategory("Navigation");

	FVoxelEditorUtilities::HideAndMoveToCategory(DetailLayout, "Tags", "Misc", { GET_MEMBER_NAME_STATIC(UActorComponent, ComponentTags) }, false);
}