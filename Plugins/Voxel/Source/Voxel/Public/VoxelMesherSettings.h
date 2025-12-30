// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMesherSettings.generated.h"

USTRUCT(BlueprintType)
struct VOXEL_API FVoxelMesherSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	bool bEnableVertexProjection = false;

	UPROPERTY(EditAnywhere, Category = "Config", AdvancedDisplay, meta = (EditCondition = "bEnableVertexProjection", UIMin = 1, UIMax = 10))
	int32 MaxSteps = 2;

	// In voxels
	UPROPERTY(EditAnywhere, Category = "Config", AdvancedDisplay, meta = (EditCondition = "bEnableVertexProjection", UIMin = 0.05, UIMax = 1))
	float Speed = 0.5;

	// In voxels
	UPROPERTY(EditAnywhere, Category = "Config", AdvancedDisplay, meta = (EditCondition = "bEnableVertexProjection", UIMin = 0.05, UIMax = 5))
	float MaxOffset = 1.f;

public:
	FORCEINLINE bool operator==(const FVoxelMesherSettings& Other) const
	{
		return
			bEnableVertexProjection == Other.bEnableVertexProjection &&
			MaxSteps == Other.MaxSteps &&
			Speed == Other.Speed &&
			MaxOffset == Other.MaxOffset;
	}
};