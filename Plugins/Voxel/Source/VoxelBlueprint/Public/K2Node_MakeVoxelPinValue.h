// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "K2Node_VoxelBaseNode.h"
#include "VoxelPinType.h"
#include "K2Node_MakeVoxelPinValue.generated.h"

UCLASS()
class VOXELBLUEPRINT_API UK2Node_MakeVoxelPinValue : public UK2Node_VoxelBaseNode
{
	GENERATED_BODY()

public:
	UK2Node_MakeVoxelPinValue();

	//~ Begin UEdGraphNode Interface
	virtual void GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;
	//~ End UEdGraphNode Interface

	//~ Begin UK2Node Interface
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void PostReconstructNode() override;
	virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const override;
	//~ End UK2Node Interface

	//~ Begin UK2Node_VoxelBaseNode Interface
	virtual bool IsPinWildcard(const UEdGraphPin& Pin) const override;
	virtual void OnPinTypeChange(UEdGraphPin& Pin, const FVoxelPinType& NewType) override;
	//~ End UK2Node_VoxelBaseNode Interface

private:
	UPROPERTY()
	FVoxelPinType CachedType;
};