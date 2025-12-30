// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNodeEvaluator.h"
#include "Sculpt/Volume/VoxelVolumeModifier.h"
#include "Sculpt/Volume/VoxelVolumeSculptGraphWrapper.h"
#include "VoxelGraphVolumeModifier.generated.h"

struct FVoxelOutputNode_OutputSculptDistance;

USTRUCT(NotBlueprintType)
struct VOXEL_API FVoxelGraphVolumeModifier : public FVoxelVolumeModifier
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	float Radius = 0;

	UPROPERTY()
	FVoxelVolumeSculptGraphWrapper Graph;

public:
	//~ Begin IVoxelVolumeModifier Interface
	virtual void Initialize_GameThread() override;
	virtual FVoxelBox GetBounds() const override;
	virtual void Apply(const FData& Data) const override;
	//~ End IVoxelVolumeModifier Interface

private:
	TVoxelNodeEvaluator<FVoxelOutputNode_OutputSculptDistance> Evaluator;
};