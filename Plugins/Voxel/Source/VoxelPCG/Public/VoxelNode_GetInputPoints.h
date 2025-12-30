// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelPointSet.h"
#include "VoxelNode_GetInputPoints.generated.h"

USTRUCT(Category = "PCG")
struct VOXELPCG_API FVoxelNode_GetInputPoints : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_OUTPUT_PIN(FVoxelPointSet, Points);

	//~ Begin FVoxelNode Interface
	virtual bool IsPureNode() const override
	{
		return true;
	}

	virtual void Compute(FVoxelGraphQuery Query) const override;
	//~ End FVoxelNode Interface
};