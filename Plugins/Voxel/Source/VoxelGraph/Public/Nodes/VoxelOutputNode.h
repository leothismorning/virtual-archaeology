// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelOutputNode.generated.h"

USTRUCT(meta = (Abstract, Internal, NodeColor = "Orange", Category = "Internal"))
struct VOXELGRAPH_API FVoxelOutputNode : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	//~ Begin FVoxelNode Interface
	virtual void PostInitialize(
		const TVoxelMap<FName, FPinRef_Input*>& NameToInputPinRefs,
		const TVoxelMap<FName, FPinRef_Output*>& NameToOutputPinRefs) override
	{
		NameToInputPin = NameToInputPinRefs;
	}
	virtual bool CanBeDuplicated() const override
	{
		return false;
	}
	virtual bool CanBeDeleted() const override
	{
		return false;
	}
	//~ End FVoxelNode Interface

public:
	TVoxelMap<FName, FPinRef_Input*> NameToInputPin;
};