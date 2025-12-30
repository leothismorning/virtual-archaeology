// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelGraphFactory.h"
#include "Sculpt/Height/VoxelHeightSculptGraph.h"
#include "Sculpt/Volume/VoxelVolumeSculptGraph.h"
#include "VoxelSculptGraphFactories.generated.h"

UCLASS()
class UVoxelHeightSculptGraphFactory : public UVoxelGraphBaseFactory
{
	GENERATED_BODY()

public:
	UVoxelHeightSculptGraphFactory(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{
		SupportedClass = UVoxelHeightSculptGraph::StaticClass();
	}
};

UCLASS()
class UVoxelVolumeSculptGraphFactory : public UVoxelGraphBaseFactory
{
	GENERATED_BODY()

public:
	UVoxelVolumeSculptGraphFactory(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{
		SupportedClass = UVoxelVolumeSculptGraph::StaticClass();
	}
};