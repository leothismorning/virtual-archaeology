// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNodeEvaluator.h"
#include "Sculpt/Height/VoxelHeightModifier.h"
#include "Sculpt/Height/VoxelHeightSculptGraphWrapper.h"
#include "VoxelGraphHeightModifier.generated.h"

struct FVoxelOutputNode_OutputSculptHeight;

USTRUCT()
struct VOXEL_API FVoxelGraphHeightModifier : public FVoxelHeightModifier
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	UPROPERTY()
	FVector2D Center = FVector2D(ForceInit);

	UPROPERTY()
	float Radius = 0;

	UPROPERTY()
	FVoxelHeightSculptGraphWrapper Graph;

public:
	//~ Begin FVoxelHeightModifier Interface
	virtual void Initialize_GameThread() override;
	virtual FVoxelBox2D GetBounds() const override;
	virtual void Apply(const FData& Data) const override;
	//~ End FVoxelHeightModifier Interface

private:
	TVoxelNodeEvaluator<FVoxelOutputNode_OutputSculptHeight> Evaluator;
};