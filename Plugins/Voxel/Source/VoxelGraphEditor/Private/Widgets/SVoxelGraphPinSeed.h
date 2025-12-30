// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "SVoxelGraphPin.h"

class SVoxelGraphPinSeed : public SVoxelGraphPin
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
};