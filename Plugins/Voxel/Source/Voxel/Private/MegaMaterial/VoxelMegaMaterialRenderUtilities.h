// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

class FVoxelMesh;
class FVoxelBufferRef;
class FVoxelMegaMaterialProxy;

struct FVoxelMegaMaterialRenderData
{
	TSharedPtr<FVoxelMegaMaterialProxy> MegaMaterialProxy;
	int32 AttributesIndicesOffset = 0;
	TVoxelArray<TSharedRef<FVoxelBufferRef>> BufferRefs;
};

struct FVoxelMegaMaterialRenderUtilities
{
	static TSharedRef<const FVoxelMegaMaterialRenderData> BuildRenderData(
		const TSharedRef<FVoxelMegaMaterialProxy>& MegaMaterialProxy,
		const TSharedRef<const FVoxelMesh>& Mesh);
};