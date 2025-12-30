// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelChunkKey.h"

class FVoxelRenderChunk;
struct FVoxelRenderSubsystem;

class FVoxelRenderNeighborSubdivider
{
public:
	FVoxelRenderSubsystem& Subsystem;
	const TVoxelArray<TSharedPtr<FVoxelRenderChunk>> RootChunks;
	const int32 MaxLOD;
	const FVoxelShouldCancel ShouldCancel;

	explicit FVoxelRenderNeighborSubdivider(
		FVoxelRenderSubsystem& Subsystem,
		const TVoxelArray<TSharedPtr<FVoxelRenderChunk>>& RootChunks);

	void Traverse();
	void ProcessNewChunks();

private:
	static constexpr int32 MaxChunksToCheck = 1024;

	FVoxelParallelTaskScope Scope;
	FVoxelSharedCriticalSection CriticalSection;
	TVoxelInlineArray<FVoxelRenderChunk*, MaxChunksToCheck> ChunksToCheck;

	TVoxelSet<FVoxelChunkKey> Chunks;
	FVoxelBitArray IsLeaf;

	TVoxelChunkedArray<const FVoxelRenderChunk*> NewChunks;

	void InitializeChunks();
	void FlushChunksToCheck();
	void CheckChunk(const FVoxelRenderChunk& Chunk);
	void SubdivideChunk(const FVoxelChunkKey& ChunkKey);
};