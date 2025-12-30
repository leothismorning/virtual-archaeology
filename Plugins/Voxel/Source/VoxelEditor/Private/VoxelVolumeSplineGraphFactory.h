// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelGraphFactory.h"
#include "Spline/VoxelVolumeSplineGraph.h"
#include "VoxelVolumeSplineGraphFactory.generated.h"

UCLASS()
class UVoxelVolumeSplineGraphFactory : public UVoxelGraphBaseFactory
{
	GENERATED_BODY()

public:
	UVoxelVolumeSplineGraphFactory(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{
		SupportedClass = UVoxelVolumeSplineGraph::StaticClass();
	}
};