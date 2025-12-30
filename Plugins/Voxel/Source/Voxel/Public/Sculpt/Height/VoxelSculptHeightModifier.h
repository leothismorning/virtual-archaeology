// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Sculpt/VoxelToolBrush.h"
#include "Sculpt/VoxelSculptMode.h"
#include "Sculpt/Height/VoxelHeightModifier.h"
#include "VoxelSculptHeightModifier.generated.h"

USTRUCT()
struct VOXEL_API FVoxelSculptHeightModifier : public FVoxelHeightModifier
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	UPROPERTY()
	FVector2D Center = FVector2D(ForceInit);

	UPROPERTY()
	float Radius = 0.f;

	UPROPERTY()
	float Strength = 0.f;

	UPROPERTY()
	EVoxelSculptMode Mode = {};

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