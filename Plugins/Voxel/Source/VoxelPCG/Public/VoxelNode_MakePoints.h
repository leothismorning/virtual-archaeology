// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelPointSet.h"
#include "Buffer/VoxelFloatBuffers.h"
#include "VoxelNode_MakePoints.generated.h"

// Create a new point set from transforms
// Useful to manually control point placement
USTRUCT(Category = "Point", meta = (ShowInShortList))
struct VOXELPCG_API FVoxelNode_MakePoints : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelTransformBuffer, Transforms, nullptr, ArrayPin);
	VOXEL_INPUT_PIN(FVoxelSeed, Seed, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelPointSet, Out);

	//~ Begin FVoxelNode Interface
	virtual bool IsPureNode() const override
	{
		return true;
	}

	virtual void Compute(FVoxelGraphQuery Query) const override;
	//~ End FVoxelNode Interface
};