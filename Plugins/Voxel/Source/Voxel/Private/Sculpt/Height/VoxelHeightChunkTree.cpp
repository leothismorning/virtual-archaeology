// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Height/VoxelHeightChunkTree.h"
#include "VoxelHeightChunkTreeImpl.ispc.generated.h"

extern "C" void* VoxelHeightChunkTreeImpl_FindChunk(
	const void* KeyToChunk,
	const int32 ChunkKeyX,
	const int32 ChunkKeyY)
{
	return static_cast<const TVoxelMap<FIntPoint, void*>*>(KeyToChunk)->FindRef(FIntPoint(
		ChunkKeyX,
		ChunkKeyY));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelHeightChunkTreeIterator::Reserve(const int32 Num)
{
	VOXEL_FUNCTION_COUNTER();

	Chunks.Reserve(Num * 4);

	FVoxelUtilities::SetNumFast(Flags_Array, Num);
	FVoxelUtilities::SetNumFast(QueryIndex_Array, Num);
	FVoxelUtilities::SetNumFast(AlphaX_Array, Num);
	FVoxelUtilities::SetNumFast(AlphaY_Array, Num);
	FVoxelUtilities::SetNumFast(PositionX_Array, Num);
	FVoxelUtilities::SetNumFast(PositionY_Array, Num);
}

template<typename FindChunkType>
FORCEINLINE void FVoxelHeightChunkTreeIterator::Set(
	const int32 Index,
	const int32 QueryIndex,
	const FVector2d& Position,
	const FindChunkType& FindChunk)
{
	const FIntPoint PositionMin = FVoxelUtilities::FloorToInt(Position);
	const FIntPoint PositionMax = FVoxelUtilities::CeilToInt(Position); // Not +1, Position is likely to be an integer
	checkVoxelSlow(PositionMax.X == PositionMin.X || PositionMax.X == PositionMin.X + 1);
	checkVoxelSlow(PositionMax.Y == PositionMin.Y || PositionMax.Y == PositionMin.Y + 1);

	const FIntPoint ChunkKeyMin = FVoxelUtilities::DivideFloor_FastLog2(PositionMin, ChunkSizeLog2);
	const FIntPoint ChunkKeyMax = FVoxelUtilities::DivideFloor_FastLog2(PositionMax, ChunkSizeLog2);

	const uint32 Flag =
		(uint32(ChunkKeyMin.X != ChunkKeyMax.X) << 0) |
		(uint32(ChunkKeyMin.Y != ChunkKeyMax.Y) << 1) |
		(uint32(PositionMin.X != PositionMax.X) << 2) |
		(uint32(PositionMin.Y != PositionMax.Y) << 3);

	Flags_Array[Index] = uint8(Flag);
	QueryIndex_Array[Index] = QueryIndex;
	AlphaX_Array[Index] = float(Position.X - PositionMin.X);
	AlphaY_Array[Index] = float(Position.Y - PositionMin.Y);
	PositionX_Array[Index] = PositionMin.X;
	PositionY_Array[Index] = PositionMin.Y;

	for (int32 ChunkIndex = 0; ChunkIndex < 4; ChunkIndex++)
	{
		if ((ChunkIndex & Flag) != ChunkIndex)
		{
			continue;
		}

		Chunks.Add_EnsureNoGrow(FindChunk(FIntPoint(
			ChunkIndex & 0b001 ? ChunkKeyMax.X : ChunkKeyMin.X,
			ChunkIndex & 0b010 ? ChunkKeyMax.Y : ChunkKeyMin.Y)));
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelHeightChunkTreeIterator FVoxelHeightChunkTreeImpl::CreateIterator(
	const TVoxelMap<FIntPoint, void*>& KeyToChunk,
	const FVoxelHeightBulkQuery& Query,
	const FVoxelHeightTransform& StampToQuery,
	const float Scale)
{
	VOXEL_FUNCTION_COUNTER_NUM(Query.Num());

#if VOXEL_DEBUG
	FVoxelHeightChunkTreeIterator Iterator2;
	{
		Iterator2.Reserve(Query.Num());

		FIntPoint LastChunkKey = FIntPoint(MAX_int32);
		const void* LastChunk = nullptr;

		int32 WriteIndex = 0;
		for (int32 IndexY = Query.Indices.Min.Y; IndexY < Query.Indices.Max.Y; IndexY++)
		{
			for (int32 IndexX = Query.Indices.Min.X; IndexX < Query.Indices.Max.X; IndexX++)
			{
				const FVector2d Position = StampToQuery.InverseTransform(Query.GetPosition(IndexX, IndexY)) / Scale;
				const int32 QueryIndex = Query.GetIndex(IndexX, IndexY);

				Iterator2.Set(WriteIndex, QueryIndex, Position, [&](const FIntPoint& ChunkKey)
				{
					if (ChunkKey != LastChunkKey)
					{
						LastChunk = KeyToChunk.FindRef(ChunkKey);
						LastChunkKey = ChunkKey;
					}
					return LastChunk;
				});

				WriteIndex++;
			}
		}
		check(WriteIndex == Query.Num());
	}
#endif

	FVoxelHeightChunkTreeIterator Iterator;
	Iterator.Reserve(Query.Num());

	Iterator.Chunks.SetNumUninitialized(Query.Num() * 4);

	int32 NumChunks = 0;
	ispc::VoxelHeightChunkTreeImpl_CreateIterator_Bulk(
	Query.ISPC(),
		StampToQuery.ISPC(),
		&KeyToChunk,
		Scale,
		Iterator.Flags_Array.GetData(),
		Iterator.QueryIndex_Array.GetData(),
		Iterator.AlphaX_Array.GetData(),
		Iterator.AlphaY_Array.GetData(),
		Iterator.PositionX_Array.GetData(),
		Iterator.PositionY_Array.GetData(),
		Iterator.Chunks.GetData(),
		&NumChunks);

	check(NumChunks <= Iterator.Chunks.Num());
	Iterator.Chunks.SetNum(NumChunks, EAllowShrinking::No);

#if VOXEL_DEBUG
	ensure(Iterator.Chunks == Iterator2.Chunks);
	ensure(Iterator.Flags_Array == Iterator2.Flags_Array);
	ensure(Iterator.QueryIndex_Array == Iterator2.QueryIndex_Array);
	ensure(Iterator.AlphaX_Array == Iterator2.AlphaX_Array);
	ensure(Iterator.AlphaY_Array == Iterator2.AlphaY_Array);
	ensure(Iterator.PositionX_Array == Iterator2.PositionX_Array);
	ensure(Iterator.PositionY_Array == Iterator2.PositionY_Array);
#endif

	return Iterator;
}

FVoxelHeightChunkTreeIterator FVoxelHeightChunkTreeImpl::CreateIterator(
	const TVoxelMap<FIntPoint, void*>& KeyToChunk,
	const FVoxelHeightSparseQuery& Query,
	const FVoxelHeightTransform& StampToQuery,
	const float Scale)
{
	VOXEL_FUNCTION_COUNTER_NUM(Query.Num());

	FVoxelHeightChunkTreeIterator Iterator;
	Iterator.Reserve(Query.Num());

	FIntPoint LastChunkKey = FIntPoint(MAX_int32);
	const void* LastChunk = nullptr;

	for (int32 Index = 0; Index < Query.Num(); Index++)
	{
		const FVector2d Position = StampToQuery.InverseTransform(Query.Positions[Index]) / Scale;

		Iterator.Set(Index, Index, Position, [&](const FIntPoint& ChunkKey)
		{
			if (ChunkKey != LastChunkKey)
			{
				LastChunk = KeyToChunk.FindRef(ChunkKey);
				LastChunkKey = ChunkKey;
			}
			return LastChunk;
		});
	}

	return Iterator;
}

FVoxelHeightChunkTreeIterator FVoxelHeightChunkTreeImpl::CreateIterator(
	const TVoxelMap<FIntPoint, void*>& KeyToChunk,
	const FVoxelDoubleVector2DBuffer& Positions)
{
	VOXEL_FUNCTION_COUNTER_NUM(Positions.Num());

	FVoxelHeightChunkTreeIterator Iterator;
	Iterator.Reserve(Positions.Num());

	FIntPoint LastChunkKey = FIntPoint(MAX_int32);
	const void* LastChunk = nullptr;

	for (int32 Index = 0; Index < Positions.Num(); Index++)
	{
		Iterator.Set(Index, Index, Positions[Index], [&](const FIntPoint& ChunkKey)
		{
			if (ChunkKey != LastChunkKey)
			{
				LastChunk = KeyToChunk.FindRef(ChunkKey);
				LastChunkKey = ChunkKey;
			}
			return LastChunk;
		});
	}

	return Iterator;
}