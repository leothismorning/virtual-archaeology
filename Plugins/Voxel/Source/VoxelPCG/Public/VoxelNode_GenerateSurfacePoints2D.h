// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelPointSet.h"
#include "Buffer/VoxelFloatBuffers.h"
#include "VoxelNode_GenerateSurfacePoints2D.generated.h"

// Generate points on a heightmap
USTRUCT(Category = "Point", DisplayName = "Generate 2D Points")
struct VOXELPCG_API FVoxelNode_Generate2DPoints : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelBox, Bounds, nullptr);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Height, nullptr);
	// A point will be placed in every cell the surface intersects with
	// This is more or less the average distance between points
	VOXEL_INPUT_PIN(float, CellSize, 100.f);
	VOXEL_INPUT_PIN(float, Jitter, 0.75f);
	VOXEL_INPUT_PIN(FVoxelSeed, Seed, nullptr, AdvancedDisplay);
	VOXEL_OUTPUT_PIN(FVoxelPointSet, Out);

	//~ Begin FVoxelNode Interface
	virtual bool IsPureNode() const override
	{
		return true;
	}

	virtual void Compute(FVoxelGraphQuery Query) const override;
	//~ End FVoxelNode Interface

public:
	static void GeneratePositions(
		const FVoxelBox& Bounds,
		int32 NumCellsX,
		int32 NumCellsY,
		float CellSize,
		float Jitter,
		const FVoxelSeed& Seed,
		FVoxelVector2DBuffer& OutPositions,
		FVoxelPointIdBuffer& OutIds);

	static void GenerateGradientPositions(
		const FVoxelVectorBuffer& Positions,
		float CellSize,
		FVoxelVector2DBuffer& OutPositions);

	static TSharedRef<FVoxelQuaternionBuffer> ComputeGradient(
		const FVoxelFloatBuffer& Heights,
		float CellSize);
};