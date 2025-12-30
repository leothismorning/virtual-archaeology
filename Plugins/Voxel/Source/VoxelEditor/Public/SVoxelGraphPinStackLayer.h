// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelStackLayer.h"

class VOXELEDITOR_API SVoxelGraphPinStackLayer : public SGraphPin
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
	FVoxelStackLayer CurrentLayer;

	TOptional<EVoxelLayerType> LayerType;
};