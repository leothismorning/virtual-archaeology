// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelChunkKey.h"
#include "VoxelSubsystem.h"
#include "VoxelRenderSubsystem.generated.h"

class FVoxelRenderChunk;
class FVoxelNaniteMaterialRenderer;
struct FVoxelRenderChunkTree;
struct FVoxelRenderChunkData;

USTRUCT()
struct VOXEL_API FVoxelRenderSubsystem : public FVoxelSubsystem
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_BODY()

public:
	bool HasChunksToSubdivide() const
	{
		return bHasChunksToSubdivide;
	}
	const FVoxelOptionalBox& GetBoundsToGenerate() const
	{
		return BoundsToGenerate;
	}
	const TVoxelArray<FVoxelChunkKey>& GetLeafChunkKeys() const
	{
		return LeafChunkKeys;
	}

	virtual ~FVoxelRenderSubsystem() override;

	//~ Begin FVoxelSubsystem Interface
	virtual bool ShouldCreateOnServer() const override { return false; }
	virtual void LoadFromPrevious(FVoxelSubsystem& InPreviousSubsystem) override;
	virtual void Initialize() override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual void Compute() override;
	virtual void Render(FVoxelRuntime& Runtime) override;
	//~ End FVoxelSubsystem Interface

private:
	FVoxelOptionalBox BoundsToGenerate;

	bool bHasChunksToSubdivide = false;
	TVoxelArray<FVoxelChunkKey> LeafChunkKeys;

	TSharedPtr<FVoxelDependencyTracker> BoundsToGenerateDependencyTracker;

	// Keep PreviousNaniteMaterialRenderer alive until we've rendered
	TSharedPtr<FVoxelNaniteMaterialRenderer> NaniteMaterialRenderer;
	TSharedPtr<FVoxelNaniteMaterialRenderer> PreviousNaniteMaterialRenderer;

	TVoxelArray<TSharedPtr<FVoxelRenderChunk>> RootChunks;
	TVoxelMap<FVoxelChunkKey, TSharedPtr<FVoxelRenderChunk>> ChunkKeyToChunk;

private:
	TVoxelChunkedArray<TSharedPtr<FVoxelRenderChunkData>> RenderDatasToDestroy;
	TVoxelChunkedArray<TSharedPtr<FVoxelRenderChunkData>> RenderDatasToRender;

	FORCEINLINE void DestroyRenderData(TSharedPtr<FVoxelRenderChunkData>& RenderData)
	{
		if (RenderData)
		{
			RenderDatasToDestroy.Add(RenderData);
			RenderData.Reset();
		}
	}

private:
	bool TryInitializeRootChunks();

private:
	struct FScreenSizeHelper
	{
		FVector CameraChunkPosition;
		double MinQuality;
		double MaxQuality;
		double ChunkToWorld;
		double QualityExponent;
		int32 MaxChunksToProcessPerPass;

		explicit FScreenSizeHelper(const FVoxelSubsystem& Subsystem);

		FORCEINLINE double GetChunkQuality(const FVoxelChunkKey ChunkKey) const
		{
			const FVoxelIntBox Bounds = ChunkKey.GetChunkKeyBounds();

			const double DistanceToCamera = Bounds.DistanceToPoint(CameraChunkPosition);
			if (DistanceToCamera == 0)
			{
				return 0;
			}

			return DistanceToCamera / FMath::Pow(double(1 << ChunkKey.LOD), QualityExponent);
		}
	};

private:
	void CollapseChunk(FVoxelRenderChunk& Chunk);
	void SubdivideChunk(FVoxelRenderChunk& Chunk);
	void DestroyChunk(FVoxelRenderChunk& Chunk);

	FVoxelRenderChunk* FindNeighbor(
		const FVoxelChunkKey& ChunkKey,
		int32 DirectionX,
		int32 DirectionY,
		int32 DirectionZ) const;

	friend class FVoxelRenderNeighborSubdivider;

private:
	void Traverse();
	void Collapse();
	void Subdivide();
	void SubdivideNeighbors();
	void FinalizeTraversal();

private:
	FVoxelFuture StartMeshingTasks();
	FVoxelFuture StartRenderTasks();
	void FinalizeRender_Nanite();
};