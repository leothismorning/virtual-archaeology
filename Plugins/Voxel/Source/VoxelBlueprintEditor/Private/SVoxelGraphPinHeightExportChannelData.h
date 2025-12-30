// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Graphs/VoxelHeightGraphBlueprintLibrary.h"

class VOXELBLUEPRINTEDITOR_API SVoxelGraphPinHeightExportChannelData : public SGraphPin
{
public:
	VOXEL_SLATE_ARGS()
	{
	};

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

protected:
	//~ Begin SGraphPin Interface
	virtual TSharedRef<SWidget> GetDefaultValueWidget() override;
	//~ End SGraphPin Interface

private:
	TSharedPtr<SComboButton> ComboButton;
	using SComboList = SListView<TSharedPtr<EVoxelHeightExportChannelType>>;
	TSharedPtr<SComboList> ComboList;
	TSharedPtr<SEditableTextBox> MetadataName;

	TArray<TSharedPtr<EVoxelHeightExportChannelType>> ChannelTypes;
	FVoxelHeightExportChannelData ChannelData;

	TSharedPtr<SMenuAnchor> AssetPickerAnchor;
};