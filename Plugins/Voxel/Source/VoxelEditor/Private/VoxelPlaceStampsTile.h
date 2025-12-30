// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"

struct FVoxelPlaceStampItem
{
	FString Name;
	FString Description;
	FString Type;
	FString BrushName;
	TSharedPtr<FAssetThumbnail> AssetThumbnail;
	TSet<FName> UserTags;
	TSet<FName> SystemTags;

	FAssetData Asset;
	TScriptInterface<IAssetFactoryInterface> ActorFactory = nullptr;
	bool bIsFavorite = false;

	TDelegate<TSet<FName>()> GetTags;
	TDelegate<FString()> GetDescription;

	void Setup();
};

class SVoxelPlaceStampsTile : public SCompoundWidget
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ATTRIBUTE(TSet<FName>, VisibleTags)
		SLATE_EVENT(TDelegate<void(bool)>, OnFavoriteStateChanged)
	};

	void Construct(const FArguments& InArgs, const TSharedPtr<FVoxelPlaceStampItem>& InItem);

protected:
	//~ Begin SCompoundWidget Interface
	virtual TOptional<EMouseCursor::Type> GetCursor() const override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	//~ End SCompoundWidget Interface

private:
	bool bIsPressed = false;
	TWeakPtr<FVoxelPlaceStampItem> WeakItem;
};