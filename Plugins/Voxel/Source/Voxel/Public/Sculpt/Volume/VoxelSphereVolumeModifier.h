// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Sculpt/VoxelSculptMode.h"
#include "Sculpt/Volume/VoxelVolumeModifier.h"
#include "VoxelSphereVolumeModifier.generated.h"

USTRUCT()
struct VOXEL_API FVoxelSphereVolumeModifier : public FVoxelVolumeModifier
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	UPROPERTY()
	FVector Center = FVector(ForceInit);

	UPROPERTY()
	float Radius = 0.f;

	UPROPERTY()
	float Smoothness = 0.f;

	UPROPERTY()
	EVoxelSculptMode Mode = {};

public:
	//~ Begin FVoxelVolumeTransaction_Modifier Interface
	virtual FVoxelBox GetBounds() const override;
	virtual void Apply(const FData& Data) const override;
	//~ End FVoxelVolumeTransaction_Modifier Interface
};