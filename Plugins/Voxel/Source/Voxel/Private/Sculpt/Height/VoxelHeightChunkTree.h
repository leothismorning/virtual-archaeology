// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFastQuadtree.h"
#include "VoxelStampQuery.h"
#include "VoxelStampTransform.h"
#include "Sculpt/Height/VoxelHeightSculptDefinitions.h"

class FVoxelHeightChunkTreeIterator : public FVoxelHeightSculptDefinitions
{
protected:
	TVoxelArray<const void*> Chunks;

	TVoxelArray<uint8> Flags_Array;
	TVoxelArray<int32> QueryIndex_Array;
	TVoxelArray<float> AlphaX_Array;
	TVoxelArray<float> AlphaY_Array;
	TVoxelArray<int32> PositionX_Array;
	TVoxelArray<int32> PositionY_Array;

	FORCEINLINE static int32 GetIndex(
		const int32 X,
		const int32 Y)
	{
		return FVoxelUtilities::Get2DIndex<int32>(ChunkSize, FIntPoint(
			X & ((1u << ChunkSizeLog2) - 1),
			Y & ((1u << ChunkSizeLog2) - 1)));
	}

	void Reserve(int32 Num);

	template<typename FindChunkType>
	void Set(
		int32 Index,
		int32 QueryIndex,
		const FVector2d& Position,
		const FindChunkType& FindChunk);

	friend struct FVoxelHeightChunkTreeImpl;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct FVoxelHeightChunkTreeImpl
{
	static FVoxelHeightChunkTreeIterator CreateIterator(
		const TVoxelMap<FIntPoint, void*>& KeyToChunk,
		const FVoxelHeightBulkQuery& Query,
		const FVoxelHeightTransform& StampToQuery,
		float Scale);

	static FVoxelHeightChunkTreeIterator CreateIterator(
		const TVoxelMap<FIntPoint, void*>& KeyToChunk,
		const FVoxelHeightSparseQuery& Query,
		const FVoxelHeightTransform& StampToQuery,
		float Scale);

	static FVoxelHeightChunkTreeIterator CreateIterator(
		const TVoxelMap<FIntPoint, void*>& KeyToChunk,
		const FVoxelDoubleVector2DBuffer& Positions);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename ChunkType>
class TVoxelHeightChunkTreeIterator : public FVoxelHeightChunkTreeIterator
{
public:
	template<
		typename Interp0DType,
		typename Interp1DType,
		typename Interp2DType>
	requires
	(
		LambdaHasSignature_V<Interp0DType, void(
			int32,
			const ChunkType*, int32)> &&

		LambdaHasSignature_V<Interp1DType, void(
			int32,
			const ChunkType*, int32,
			const ChunkType*, int32,
			float)> &&

		LambdaHasSignature_V<Interp2DType, void(
			int32,
			const ChunkType*, int32,
			const ChunkType*, int32,
			const ChunkType*, int32,
			const ChunkType*, int32,
			float,
			float)>
	)
	void Iterate(
		Interp0DType Interp0,
		Interp1DType Interp1,
		Interp2DType Interp2) const
	{
		VOXEL_FUNCTION_COUNTER_NUM(Flags_Array.Num());

		int32 ChunkIndex = 0;
		for (int32 Index = 0; Index < Flags_Array.Num(); Index++)
		{
			const uint8 Flag = Flags_Array[Index];
			const int32 QueryIndex = QueryIndex_Array[Index];
			const float AlphaX = AlphaX_Array[Index];
			const float AlphaY = AlphaY_Array[Index];
			const int32 PositionX = PositionX_Array[Index];
			const int32 PositionY = PositionY_Array[Index];

			const uint32 HasMaxPosition = Flag >> 2;
			const uint32 HasMaxChunk = Flag & 0b11;
			checkVoxelSlow((HasMaxPosition & HasMaxChunk) == HasMaxChunk);

			switch (HasMaxPosition)
			{
			default: VOXEL_ASSUME(false);
			case 0b00:
			{
				const ChunkType* Chunk0;

				switch (HasMaxChunk)
				{
				default: VOXEL_ASSUME(false);
				case 0b00:
				{
					Chunk0 = static_cast<const ChunkType*>(Chunks[ChunkIndex++]);
				}
				break;
				}

				const int32 Index0 = GetIndex(PositionX + 0, PositionY);

				Interp0(QueryIndex, Chunk0, Index0);
			}
			break;
			case 0b01:
			{
				const ChunkType* Chunk0;
				const ChunkType* Chunk1;

				switch (HasMaxChunk)
				{
				default: VOXEL_ASSUME(false);
				case 0b00:
				{
					Chunk0 = static_cast<const ChunkType*>(Chunks[ChunkIndex++]);
					Chunk1 = Chunk0;
				}
				break;
				case 0b01:
				{
					Chunk0 = static_cast<const ChunkType*>(Chunks[ChunkIndex++]);
					Chunk1 = static_cast<const ChunkType*>(Chunks[ChunkIndex++]);
				}
				break;
				}

				const int32 Index0 = GetIndex(PositionX + 0, PositionY);
				const int32 Index1 = GetIndex(PositionX + 1, PositionY);

				Interp1(
					QueryIndex,
					Chunk0, Index0,
					Chunk1, Index1,
					AlphaX);
			}
			break;
			case 0b10:
			{
				const ChunkType* Chunk0;
				const ChunkType* Chunk1;

				switch (HasMaxChunk)
				{
				default: VOXEL_ASSUME(false);
				case 0b00:
				{
					Chunk0 = static_cast<const ChunkType*>(Chunks[ChunkIndex++]);
					Chunk1 = Chunk0;
				}
				break;
				case 0b10:
				{
					Chunk0 = static_cast<const ChunkType*>(Chunks[ChunkIndex++]);
					Chunk1 = static_cast<const ChunkType*>(Chunks[ChunkIndex++]);
				}
				break;
				}

				const int32 Index0 = GetIndex(PositionX, PositionY + 0);
				const int32 Index1 = GetIndex(PositionX, PositionY + 1);

				Interp1(
					QueryIndex,
					Chunk0, Index0,
					Chunk1, Index1,
					AlphaY);
			}
			break;
			case 0b11:
			{
				const ChunkType* Chunk0;
				const ChunkType* Chunk1;
				const ChunkType* Chunk2;
				const ChunkType* Chunk3;

				switch (HasMaxChunk)
				{
				default: VOXEL_ASSUME(false);
				case 0b00:
				{
					Chunk0 = static_cast<const ChunkType*>(Chunks[ChunkIndex++]);
					Chunk1 = Chunk0;
					Chunk2 = Chunk0;
					Chunk3 = Chunk0;
				}
				break;
				case 0b01:
				{
					Chunk0 = static_cast<const ChunkType*>(Chunks[ChunkIndex++]);
					Chunk1 = static_cast<const ChunkType*>(Chunks[ChunkIndex++]);
					Chunk2 = Chunk0;
					Chunk3 = Chunk1;
				}
				break;
				case 0b10:
				{
					Chunk0 = static_cast<const ChunkType*>(Chunks[ChunkIndex++]);
					Chunk1 = Chunk0;
					Chunk2 = static_cast<const ChunkType*>(Chunks[ChunkIndex++]);
					Chunk3 = Chunk2;
				}
				break;
				case 0b11:
				{
					Chunk0 = static_cast<const ChunkType*>(Chunks[ChunkIndex++]);
					Chunk1 = static_cast<const ChunkType*>(Chunks[ChunkIndex++]);
					Chunk2 = static_cast<const ChunkType*>(Chunks[ChunkIndex++]);
					Chunk3 = static_cast<const ChunkType*>(Chunks[ChunkIndex++]);
				}
				break;
				}

				const int32 Index0 = GetIndex(PositionX + 0, PositionY + 0);
				const int32 Index1 = GetIndex(PositionX + 1, PositionY + 0);
				const int32 Index2 = GetIndex(PositionX + 0, PositionY + 1);
				const int32 Index3 = GetIndex(PositionX + 1, PositionY + 1);

				Interp2(
					QueryIndex,
					Chunk0, Index0,
					Chunk1, Index1,
					Chunk2, Index2,
					Chunk3, Index3,
					AlphaX,
					AlphaY);
			}
			break;
			}
		}
		checkVoxelSlow(ChunkIndex == Chunks.Num());
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename ChunkType>
class TVoxelHeightChunkTree : public FVoxelHeightSculptDefinitions
{
public:
	FORCEINLINE bool IsEmpty() const
	{
		return KeyToChunk.Num() == 0;
	}
	FORCEINLINE const ChunkType* FindChunk(const FIntPoint& ChunkKey) const
	{
		const TVoxelRefCountPtr<const ChunkType>* Result = KeyToChunk.Find(ChunkKey);
		if (!Result)
		{
			return nullptr;
		}

		return Result->GetReference();
	}
	FORCEINLINE const TVoxelMap<FIntPoint, TVoxelRefCountPtr<const ChunkType>>& GetKeyToChunk() const
	{
		return KeyToChunk;
	}

public:
	FVoxelBox2D GetBounds() const
	{
		ensure(!IsEmpty());
		return FVoxelBox2D(FVector2D(MinKey) * ChunkSize, FVector2D(MaxKey + 1) * ChunkSize);
	}
	bool HasChunks(const FVoxelBox2D& Bounds) const
	{
		VOXEL_FUNCTION_COUNTER();

		const FVoxelIntBox2D LocalBounds = FVoxelIntBox2D::FromFloatBox_WithPadding(Bounds
			.Scale(1. / double(ChunkSize))
			.IntersectWith(FVoxelBox2D(MIN_int32, MAX_int32)));

		bool bHasChunks = false;
		Quadtree.TraverseBounds(LocalBounds, [&](const TVoxelFastQuadtree<>::FNodeRef& NodeRef)
		{
			if (NodeRef.GetHeight() == 0)
			{
				bHasChunks = true;
				return EVoxelIterateTree::Stop;
			}

			return EVoxelIterateTree::Continue;
		});

		return bHasChunks;
	}

public:
	TVoxelHeightChunkTreeIterator<ChunkType> CreateIterator(
		const FVoxelHeightBulkQuery& Query,
		const FVoxelHeightTransform& StampToQuery,
		const float Scale) const
	{
		checkVoxelSlow(HasChunks(StampToQuery.InverseTransform(Query.GetBounds()).Scale(1. / Scale)));

		return ReinterpretCastRef<TVoxelHeightChunkTreeIterator<ChunkType>>(FVoxelHeightChunkTreeImpl::CreateIterator(
			ReinterpretCastRef<TVoxelMap<FIntPoint, void*>>(KeyToChunk),
			Query,
			StampToQuery,
			Scale));
	}
	TVoxelHeightChunkTreeIterator<ChunkType> CreateIterator(
		const FVoxelHeightSparseQuery& Query,
		const FVoxelHeightTransform& StampToQuery,
		const float Scale) const
	{
		checkVoxelSlow(HasChunks(StampToQuery.InverseTransform(Query.PositionBounds).Scale(1. / Scale)));

		return ReinterpretCastRef<TVoxelHeightChunkTreeIterator<ChunkType>>(FVoxelHeightChunkTreeImpl::CreateIterator(
			ReinterpretCastRef<TVoxelMap<FIntPoint, void*>>(KeyToChunk),
			Query,
			StampToQuery,
			Scale));
	}
	TVoxelHeightChunkTreeIterator<ChunkType> CreateIterator(const FVoxelDoubleVector2DBuffer& Positions) const
	{
		return ReinterpretCastRef<TVoxelHeightChunkTreeIterator<ChunkType>>(FVoxelHeightChunkTreeImpl::CreateIterator(
			ReinterpretCastRef<TVoxelMap<FIntPoint, void*>>(KeyToChunk),
			Positions));
	}

public:
	void Serialize(FArchive& Ar)
	{
		VOXEL_FUNCTION_COUNTER();

		using FVersion = DECLARE_VOXEL_VERSION
		(
			FirstVersion,
			LegacyVersion
		);

		int32 Version = FVersion::LatestVersion;
		Ar << Version;

		TVoxelArray<FIntPoint> ChunkKeys = KeyToChunk.KeyArray();
		Ar << ChunkKeys;

		if (Ar.IsLoading())
		{
			MinKey = FIntPoint(MAX_int32);
			MaxKey = FIntPoint(MIN_int32);

			ensure(KeyToChunk.Num() == 0);
			KeyToChunk.Reset();
			KeyToChunk.Reserve(ChunkKeys.Num());

			for (const FIntPoint& ChunkKey : ChunkKeys)
			{
				MinKey = FVoxelUtilities::ComponentMin(MinKey, ChunkKey);
				MaxKey = FVoxelUtilities::ComponentMax(MaxKey, ChunkKey);

				KeyToChunk.Add_CheckNew(ChunkKey, new ChunkType());
			}

			for (const FIntPoint& ChunkKey : ChunkKeys)
			{
				Quadtree.TraverseBounds(FVoxelIntBox2D(ChunkKey), [&](const TVoxelFastQuadtree<>::FNodeRef& NodeRef)
				{
					if (NodeRef.GetHeight() > 0)
					{
						Quadtree.CreateAllChildren(NodeRef);
					}
				});
			}
		}

		for (const auto& It : KeyToChunk)
		{
			ConstCast(*It.Value).Serialize(Ar);
		}
	}
	void CopyFrom(const TVoxelHeightChunkTree& Other)
	{
		VOXEL_FUNCTION_COUNTER();

		MinKey = Other.MinKey;
		MaxKey = Other.MaxKey;
		Quadtree.CopyFrom(Other.Quadtree);
		KeyToChunk = Other.KeyToChunk;
	}
	void SetChunk(
		const FIntPoint& ChunkKey,
		const TVoxelRefCountPtr<ChunkType>& Chunk)
	{
		VOXEL_FUNCTION_COUNTER();

		if (!Chunk)
		{
			KeyToChunk.Remove(ChunkKey);
			return;
		}

		if (KeyToChunk.Contains(ChunkKey))
		{
			KeyToChunk.FindChecked(ChunkKey) = Chunk;
		}
		else
		{
			MinKey = FVoxelUtilities::ComponentMin(MinKey, ChunkKey);
			MaxKey = FVoxelUtilities::ComponentMax(MaxKey, ChunkKey);

			Quadtree.TraverseBounds(FVoxelIntBox2D(ChunkKey), [&](const TVoxelFastQuadtree<>::FNodeRef& NodeRef)
			{
				if (NodeRef.GetHeight() > 0)
				{
					Quadtree.CreateAllChildren(NodeRef);
				}
			});

			KeyToChunk.Add_EnsureNew(ChunkKey, Chunk);
		}
	}

private:
	FIntPoint MinKey = FIntPoint(MAX_int32);
	FIntPoint MaxKey = FIntPoint(MIN_int32);
	TVoxelFastQuadtree<> Quadtree{ 30 };
	TVoxelMap<FIntPoint, TVoxelRefCountPtr<const ChunkType>> KeyToChunk;
};