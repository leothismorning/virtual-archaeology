// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelPointSet.h"
#include "Buffer/VoxelBaseBuffers.h"
#include "VoxelPointNodes.generated.h"

USTRUCT(Category = "Point")
struct VOXELPCG_API FVoxelNode_FilterPoints : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelPointSet, In, nullptr);
	VOXEL_INPUT_PIN(FVoxelBoolBuffer, KeepPoint, true);
	VOXEL_OUTPUT_PIN(FVoxelPointSet, Out);

	//~ Begin FVoxelNode Interface
	virtual void Compute(FVoxelGraphQuery Query) const override;
	//~ End FVoxelNode Interface
};

USTRUCT(Category = "Point")
struct VOXELPCG_API FVoxelNode_DensityFilter : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelPointSet, In, nullptr);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Density, 1.f);
	VOXEL_INPUT_PIN(FVoxelSeed, Seed, nullptr, AdvancedDisplay);
	VOXEL_OUTPUT_PIN(FVoxelPointSet, Out);

	//~ Begin FVoxelNode Interface
	virtual void Compute(FVoxelGraphQuery Query) const override;
	//~ End FVoxelNode Interface
};