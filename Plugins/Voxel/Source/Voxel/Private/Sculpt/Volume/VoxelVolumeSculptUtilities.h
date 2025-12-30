// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Sculpt/Volume/VoxelVolumeSculptDefinitions.h"

struct FVoxelBuffer;
struct FVoxelSurfaceTypeBlend;
struct FVoxelVolumeDistanceChunk;
struct FVoxelVolumeMetadataChunk;
struct FVoxelVolumeSurfaceTypeChunk;
struct FVoxelVolumeFastDistanceChunk;

struct FVoxelVolumeSculptUtilities : FVoxelVolumeSculptDefinitions
{
public:
	static void DiffDistances(
		TConstVoxelArrayView<float> Distances,
		TConstVoxelArrayView<float> PreviousDistances,
		const FIntVector& Size,
		float VoxelSize,
		TVoxelArray<float>& AdditiveDistances,
		TVoxelArray<float>& SubtractiveDistances);

public:
	static TVoxelRefCountPtr<FVoxelVolumeFastDistanceChunk> CreateFastDistanceChunk(
		const FIntVector& ChunkKey,
		TConstVoxelArrayView<float> Distances,
		const FIntVector& Size,
		float VoxelSize);

public:
	static TVoxelRefCountPtr<FVoxelVolumeDistanceChunk> CreateDistanceChunk_NoDiffing(
		const FIntVector& ChunkKey,
		TConstVoxelArrayView<float> Distances,
		const FIntVector& Size,
		float VoxelSize);

	static TVoxelRefCountPtr<FVoxelVolumeDistanceChunk> CreateDistanceChunk_Diffing(
		const FIntVector& ChunkKey,
		TConstVoxelArrayView<float> AdditiveDistances,
		TConstVoxelArrayView<float> SubtractiveDistances,
		const FIntVector& Size,
		float VoxelSize);

public:
	static TVoxelRefCountPtr<FVoxelVolumeSurfaceTypeChunk> CreateSurfaceTypeChunk(
		const FIntVector& ChunkKey,
		const TVoxelOptional<TConstVoxelArrayView<int32>>& Indirection,
		TConstVoxelArrayView<float> Alphas,
		TConstVoxelArrayView<FVoxelSurfaceTypeBlend> SurfaceTypes,
		const FIntVector& Size);

	static TVoxelRefCountPtr<FVoxelVolumeMetadataChunk> CreateMetadataChunk(
		const FIntVector& ChunkKey,
		const TVoxelOptional<TConstVoxelArrayView<int32>>& Indirection,
		TConstVoxelArrayView<float> Alphas,
		const FVoxelBuffer& Buffer,
		const FIntVector& Size);
};