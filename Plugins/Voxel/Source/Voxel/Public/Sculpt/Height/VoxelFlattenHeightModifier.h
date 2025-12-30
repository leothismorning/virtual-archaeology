// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Sculpt/VoxelToolBrush.h"
#include "Sculpt/VoxelLevelToolType.h"
#include "Sculpt/Height/VoxelHeightModifier.h"
#include "VoxelFlattenHeightModifier.generated.h"

USTRUCT()
struct VOXEL_API FVoxelFlattenHeightModifier : public FVoxelHeightModifier
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	UPROPERTY()
	FVector2D Center = FVector2D(ForceInit);

	UPROPERTY()
	float Radius = 0.f;

	UPROPERTY()
	float Falloff = 0.f;

	UPROPERTY()
	EVoxelLevelToolType Type = {};

	UPROPERTY()
	float TargetHeight = 0;

	UPROPERTY()
	FVoxelToolBrush Brush;

public:
	//~ Begin FVoxelHeightModifier Interface
	virtual void Initialize_GameThread() override;
	virtual FVoxelBox2D GetBounds() const override;
	virtual void Apply(const FData& Data) const override;
	//~ End FVoxelHeightModifier Interface

private:
	TSharedPtr<const FVoxelToolRuntimeBrush> RuntimeBrush;
};