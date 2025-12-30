// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Sculpt/VoxelToolBrush.h"
#include "Sculpt/Volume/VoxelVolumeModifier.h"
#include "VoxelAngleVolumeModifier.generated.h"

UENUM(BlueprintType)
enum class EVoxelSDFMergeMode : uint8
{
	// Additive mode: will only grow the surface
	Union,
	// Destructive mode: will only shrink the surface
	Intersection,
	// Will add and remove at the same time
	Override
};

USTRUCT()
struct VOXEL_API FVoxelAngleVolumeModifier : public FVoxelVolumeModifier
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
	FPlane Plane = FPlane(ForceInit);

	UPROPERTY()
	EVoxelSDFMergeMode MergeMode = {};

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