// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Misc/TextFilter.h"

struct FVoxelPlaceStampItem;

class SVoxelPlaceStampsTab : public SCompoundWidget
{
public:
	using FContentSourceTextFilter = TTextFilter<TSharedPtr<FVoxelPlaceStampItem>>;
	using FCollectItemsFunction = TDelegate<void(TArray<TSharedPtr<FVoxelPlaceStampItem>>&)>;

public:
	VOXEL_SLATE_ARGS()
	{
	};

	void Construct(const FArguments& InArgs, const TSharedPtr<SDockTab>& ParentTab);
	void FocusSearchBox() const;

private:
	TSharedRef<SWidget> CreateCategoriesWidget();
	void SetActiveTab(FName Name);
	void UpdateContentForCategory(FName CategoryName);
	void UpdateTags(bool bResetVisibleTags);
	void UpdateFilteredItems();
	void UpdateFavorites();

private:
	void OnAssetsAdded(TArrayView<const FAssetData> AssetDatas);
	void OnAssetRenamed(const FAssetData& AssetData, const FString& OldName);
	void OnAssetUpdated(const FAssetData& AssetData);
	void OnAssetRemoved(const FAssetData& AssetData);

private:
	static TSharedPtr<FVoxelPlaceStampItem> ConstructGraph(const FAssetData& AssetData);
	static TSharedPtr<FVoxelPlaceStampItem> ConstructMesh(const FAssetData& AssetData);
	static TSharedPtr<FVoxelPlaceStampItem> ConstructHeightmap(const FAssetData& AssetData);
	static TSharedPtr<FVoxelPlaceStampItem> ConstructSculpt(const FAssetData& AssetData);
	static TSharedPtr<FVoxelPlaceStampItem> ConstructShape(const FAssetData& AssetData);

private:
	void FillFavoritesList();
	void CollectAllFavorites(TArray<TSharedPtr<FVoxelPlaceStampItem>>& OutItems);
	bool UseSystemTags() const;
	static void CollectAllRecent(TArray<TSharedPtr<FVoxelPlaceStampItem>>& OutItems);

public:
	static FLinearColor GetStampTagColor(FName StampTag);
	static TSharedPtr<SVoxelPlaceStampsTab> GetDrawerWidget();

	static void RegisterDrawer();
	static void UnregisterDrawer();

private:
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<STextBlock> MessageTextBlock;
	TSharedPtr<SWrapBox> TagsBox;
	TSharedPtr<SWidgetSwitcher> WidgetSwitcher;
	TSharedPtr<STileView<TSharedPtr<FVoxelPlaceStampItem>>> TileView;

	TSet<FSoftObjectPath> Favorites;

private:
	TArray<TSharedPtr<FVoxelPlaceStampItem>> AllCategoryItems;
	TArray<TSharedPtr<FVoxelPlaceStampItem>> FilteredCategoryItems;
	TMap<FSoftObjectPath, TSharedPtr<FVoxelPlaceStampItem>> TrackedAssetToItem;

	TSharedPtr<FContentSourceTextFilter> Filter;

private:
	TMap<FName, FCollectItemsFunction> CategoryToCollectFunction;
	FName ActiveTabName;
	TSet<FName> Tags;
	TSet<FName> VisibleTags;
};