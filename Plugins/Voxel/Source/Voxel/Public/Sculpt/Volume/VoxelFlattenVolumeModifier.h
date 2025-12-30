// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Sculpt/VoxelLevelToolType.h"
#include "Sculpt/Volume/VoxelVolumeModifier.h"
#include "VoxelFlattenVolumeModifier.generated.h"

USTRUCT()
struct VOXEL_API FVoxelFlattenVolumeModifier : public FVoxelVolumeModifier
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	UPROPERTY()
	FVector Center = FVector(ForceInit);

	UPROPERTY()
	FVector Normal = FVector(ForceInit);

	UPROPERTY()
	float Radius = 0.f;

	UPROPERTY()
	float Height = 0.f;

	UPROPERTY()
	float Falloff = 0.f;

	UPROPERTY()
	EVoxelLevelToolType Type = {};

public:
	//~ Begin FVoxelVolumeTransaction_Modifier Interface
	virtual FVoxelBox GetBounds() const override;
	virtual void Apply(const FData& Data) const override;
	//~ End FVoxelVolumeTransaction_Modifier Interface
};