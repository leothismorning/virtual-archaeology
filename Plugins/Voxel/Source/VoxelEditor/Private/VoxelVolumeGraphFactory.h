// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelGraphFactory.h"
#include "Graphs/VoxelVolumeGraph.h"
#include "VoxelVolumeGraphFactory.generated.h"

UCLASS()
class UVoxelVolumeGraphFactory : public UVoxelGraphBaseFactory
{
	GENERATED_BODY()

public:
	UVoxelVolumeGraphFactory(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{
		SupportedClass = UVoxelVolumeGraph::StaticClass();
	}
};