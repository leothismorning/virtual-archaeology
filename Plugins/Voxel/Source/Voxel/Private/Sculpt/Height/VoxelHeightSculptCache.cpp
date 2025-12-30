// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Height/VoxelHeightSculptCache.h"
#include "Sculpt/Height/VoxelHeightSculptInnerData.h"
#include "Sculpt/Height/VoxelHeightSculptStamp.h"
#include "VoxelQuery.h"

DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelHeightSculptCache_Memory);

int64 FVoxelHeightSculptPreviousChunk::GetAllocatedSize() const
{
	return sizeof(*this);
}

FVoxelHeightSculptPreviousChunk::FVoxelHeightSculptPreviousChunk()
{
	UpdateStats();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelHeightSculptCache::FVoxelHeightSculptCache(
	const FTransform2d& SculptToWorld,
	const FVoxelHeightSculptDataId SculptDataId)
	: SculptToWorld(SculptToWorld)
	, SculptDataId(SculptDataId)
{
}

FVoxelHeightSculptCache::~FVoxelHeightSculptCache()
{
	VOXEL_FUNCTION_COUNTER();

	ChunkKeyToChunk_RequiresLock.Empty();
}

TSharedRef<const FVoxelHeightSculptPreviousChunk> FVoxelHeightSculptCache::ComputeChunk(
	const FVoxelLayers& Layers,
	const FVoxelSurfaceTypeTable& SurfaceTypeTable,
	const FVoxelWeakStackLayer& WeakLayer,
	const FIntPoint& ChunkKey)
{
	VOXEL_FUNCTION_COUNTER();

	{
		VOXEL_SCOPE_READ_LOCK(CriticalSection);

		const TSharedPtr<const FVoxelHeightSculptPreviousChunk> Chunk = ChunkKeyToChunk_RequiresLock.FindRef(ChunkKey);
		if (Chunk &&
			!Chunk->DependencyTracker->IsInvalidated())
		{
			return Chunk.ToSharedRef();
		}
	}

	FVoxelDoubleVector2DBuffer Positions;
	{
		VOXEL_SCOPE_COUNTER("Positions");

		Positions.Allocate(ChunkCount);

		for (int32 IndexY = 0; IndexY < ChunkSize; IndexY++)
		{
			for (int32 IndexX = 0; IndexX < ChunkSize; IndexX++)
			{
				const int32 Index = FVoxelUtilities::Get2DIndex<int32>(
					ChunkSize,
					IndexX,
					IndexY);

				const FVector2D Position = SculptToWorld.TransformPoint(FVector2D(
					ChunkKey.X * ChunkSize + IndexX,
					ChunkKey.Y * ChunkSize + IndexY));

				Positions.Set(Index, Position);
			}
		}
	}

	const TSharedRef<FVoxelHeightSculptPreviousChunk> NewChunk = MakeShareable(new FVoxelHeightSculptPreviousChunk());

	FVoxelDependencyCollector DependencyCollector(STATIC_FNAME("FVoxelHeightSculptPreviousChunk"));

	const TVoxelUniqueFunction<bool(const FVoxelStampRuntime&)> ShouldStopTraversal_BeforeStamp = [&](const FVoxelStampRuntime& NextStamp)
	{
		const FVoxelHeightSculptStampRuntime* SculptStamp = NextStamp.As<FVoxelHeightSculptStampRuntime>();
		if (!SculptStamp ||
			!SculptStamp->InnerData)
		{
			return false;
		}

		return SculptStamp->SculptDataId == SculptDataId;
	};

	FVoxelQuery Query(
		0,
		Layers,
		SurfaceTypeTable,
		DependencyCollector);

	Query.ShouldStopTraversal_BeforeStamp = &ShouldStopTraversal_BeforeStamp;

	const FVoxelFloatBuffer QueriedHeights = Query.SampleHeightLayer(WeakLayer, Positions);

	FVoxelUtilities::Memcpy(NewChunk->Heights, QueriedHeights.View());

	NewChunk->DependencyTracker = DependencyCollector.Finalize(nullptr, {});

	{
		VOXEL_SCOPE_WRITE_LOCK(CriticalSection);
		ChunkKeyToChunk_RequiresLock.FindOrAdd(ChunkKey) = NewChunk;
	}

	return NewChunk;
}