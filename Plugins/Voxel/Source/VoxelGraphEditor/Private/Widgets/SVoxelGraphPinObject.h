// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "KismetPins/SGraphPinObject.h"
#include "SVoxelGraphPin.h"

class SVoxelGraphPinObject : public SGraphPinObject
{
public:
	VOXEL_SLATE_ARGS()
	{
	};

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

	//~ Begin SGraphPinObject Interface
	virtual TSharedRef<SWidget> GetDefaultValueWidget() override;
	virtual void OnAssetSelectedFromPicker(const FAssetData& AssetData) override;
	VOXEL_PIN_MAKE_EDITABLE_LABEL(SVoxelGraphPinObject, SGraphPinObject)
	//~ End SGraphPinObject Interface
};