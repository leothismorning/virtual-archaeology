// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

class FVoxelLayers;
class FVoxelSurfaceTypeTable;
struct FVoxelWeakStackLayer;
struct FVoxelVectorBuffer;
struct FVoxelSurfaceTypeBlend;
struct FVoxelDoubleVectorBuffer;

struct VOXEL_API FVoxelSmartSurfaceTypeUtilities
{
	static void Resolve(
		int32 LOD,
		const FVoxelWeakStackLayer& WeakLayer,
		FVoxelDependencyCollector& DependencyCollector,
		const FVoxelLayers& Layers,
		const FVoxelSurfaceTypeTable& SurfaceTypeTable,
		const FVoxelDoubleVectorBuffer& VertexPositions,
		const FVoxelVectorBuffer& VertexNormals,
		TVoxelArrayView<FVoxelSurfaceTypeBlend> SurfaceTypeBlends);
};