// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Sculpt/Height/VoxelHeightSculptDefinitions.h"

struct FVoxelBuffer;
struct FVoxelSurfaceTypeBlend;
struct FVoxelHeightHeightChunk;
struct FVoxelHeightMetadataChunk;
struct FVoxelHeightSurfaceTypeChunk;

struct FVoxelHeightSculptUtilities : FVoxelHeightSculptDefinitions
{
	static TVoxelRefCountPtr<FVoxelHeightHeightChunk> CreateHeightChunk(
		const FIntPoint& ChunkKey,
		TConstVoxelArrayView<float> Heights,
		const FIntPoint& Size,
		float ScaleZ,
		float OffsetZ);

	static TVoxelRefCountPtr<FVoxelHeightSurfaceTypeChunk> CreateSurfaceTypeChunk(
		const FIntPoint& ChunkKey,
		TConstVoxelArrayView<float> Alphas,
		TConstVoxelArrayView<FVoxelSurfaceTypeBlend> SurfaceTypes,
		const FIntPoint& Size);

	static TVoxelRefCountPtr<FVoxelHeightMetadataChunk> CreateMetadataChunk(
		const FIntPoint& ChunkKey,
		TConstVoxelArrayView<float> Alphas,
		const FVoxelBuffer& Buffer,
		const FIntPoint& Size);
};