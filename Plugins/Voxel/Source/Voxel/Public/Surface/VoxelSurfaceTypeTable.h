// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelSurfaceType.h"

class FVoxelSmartSurfaceProxy;

class VOXEL_API FVoxelSurfaceTypeTable
{
public:
	const TVoxelArray<FVoxelSurfaceType> InvisibleSurfaceTypes;
	const TSharedRef<FVoxelDependency> InvisibleSurfaceTypesDependency;

	const TVoxelMap<FVoxelSurfaceType, TSharedPtr<FVoxelSmartSurfaceProxy>> SurfaceTypeToSmartSurfaceProxy;

public:
	static TSharedRef<FVoxelSurfaceTypeTable> Get();
	static void Refresh();

private:
	FVoxelSurfaceTypeTable(
		TVoxelArray<FVoxelSurfaceType>&& InvisibleSurfaces,
		const TSharedRef<FVoxelDependency>& InvisibleSurfacesDependency,
		TVoxelMap<FVoxelSurfaceType, TSharedPtr<FVoxelSmartSurfaceProxy>>&& SurfaceTypeToSmartSurfaceProxy)
		: InvisibleSurfaceTypes(MoveTemp(InvisibleSurfaces))
		, InvisibleSurfaceTypesDependency(InvisibleSurfacesDependency)
		, SurfaceTypeToSmartSurfaceProxy(MoveTemp(SurfaceTypeToSmartSurfaceProxy))
	{
	}

	friend class FVoxelSurfaceTypeTableManager;
};