// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelGraphFactory.h"
#include "Spline/VoxelHeightSplineGraph.h"
#include "VoxelHeightSplineGraphFactory.generated.h"

UCLASS()
class UVoxelHeightSplineGraphFactory : public UVoxelGraphBaseFactory
{
	GENERATED_BODY()

public:
	UVoxelHeightSplineGraphFactory(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{
		SupportedClass = UVoxelHeightSplineGraph::StaticClass();
	}
};