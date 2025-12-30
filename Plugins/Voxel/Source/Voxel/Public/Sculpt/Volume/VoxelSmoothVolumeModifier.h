// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Sculpt/VoxelToolBrush.h"
#include "Sculpt/Volume/VoxelVolumeModifier.h"
#include "VoxelSmoothVolumeModifier.generated.h"

class FVoxelToolRuntimeBrush;

USTRUCT()
struct VOXEL_API FVoxelSmoothVolumeModifier : public FVoxelVolumeModifier
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	UPROPERTY()
	FVector Center = FVector(ForceInit);

	UPROPERTY()
	float Radius = 0.f;

	UPROPERTY()
	float Strength = 0.f;

	UPROPERTY()
	FVoxelToolBrush Brush;

public:
	//~ Begin FVoxelVolumeTransaction_Modifier Interface
	virtual void Initialize_GameThread() override;
	virtual FVoxelBox GetBounds() const override;
	virtual void Apply(const FData& Data) const override;
	//~ End FVoxelVolumeTransaction_Modifier Interface

private:
	TSharedPtr<const FVoxelToolRuntimeBrush> RuntimeBrush;
};