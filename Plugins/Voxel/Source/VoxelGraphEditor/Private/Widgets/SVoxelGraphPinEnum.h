// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "SVoxelGraphPin.h"

class SVoxelGraphPinEnum : public SVoxelGraphPin
{
public:
	VOXEL_SLATE_ARGS()
	{
	};

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

	//~ Begin SGraphPin Interface
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;
	//~ End SGraphPin Interface
};