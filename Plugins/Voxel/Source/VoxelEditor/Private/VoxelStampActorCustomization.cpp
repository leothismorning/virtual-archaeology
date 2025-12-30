// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelStampActor.h"

VOXEL_CUSTOMIZE_CLASS(AVoxelStampActor)(IDetailLayoutBuilder& DetailLayout)
{
	FVoxelEditorUtilities::EnableRealtime();

	DetailLayout.HideCategory("Rendering");
	DetailLayout.HideCategory("Replication");
	DetailLayout.HideCategory("Input");
	DetailLayout.HideCategory("Collision");
	DetailLayout.HideCategory("LOD");
	DetailLayout.HideCategory("HLOD");
	DetailLayout.HideCategory("Cooking");
	DetailLayout.HideCategory("DataLayers");
	DetailLayout.HideCategory("Networking");
	DetailLayout.HideCategory("Physics");

	FVoxelEditorUtilities::HideAndMoveToCategory(DetailLayout, "Actor", "Misc", { GET_MEMBER_NAME_STATIC(AActor, Tags) }, false);
	FVoxelEditorUtilities::HideAndMoveToCategory(DetailLayout, "WorldPartition", "Misc");
	FVoxelEditorUtilities::HideAndMoveToCategory(DetailLayout, "LevelInstance", "Misc");

	TArray<FName> Categories;
	DetailLayout.GetCategoryNames(Categories);
	for (const FName Category : Categories)
	{
		FString CategoryName = Category.ToString();
		if (!CategoryName.RemoveFromStart("Voxel ") ||
			CategoryName.Contains("|"))
		{
			continue;
		}

		DetailLayout.EditCategory(Category, FText::FromString(CategoryName));
	}
}