// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFastOctree.h"
#include "Sculpt/Volume/VoxelVolumeSculptVersion.h"
#include "Sculpt/Volume/VoxelVolumeChunkTreeIterator.h"

template<typename ChunkType>
class TVoxelVolumeChunkTree : public FVoxelVolumeSculptDefinitions
{
public:
	FORCEINLINE bool IsEmpty() const
	{
		return KeyToChunk.Num() == 0;
	}
	FORCEINLINE const ChunkType* FindChunk(const FIntVector& ChunkKey) const
	{
		const TVoxelRefCountPtr<const ChunkType>* Result = KeyToChunk.Find(ChunkKey);
		if (!Result)
		{
			return nullptr;
		}

		return Result->GetReference();
	}

public:
	FVoxelBox GetBounds() const
	{
		ensure(!IsEmpty());
		return FVoxelBox(FVector(MinKey) * ChunkSize, FVector(MaxKey + 1) * ChunkSize);
	}
	bool HasChunks(const FVoxelBox& Bounds) const
	{
		VOXEL_FUNCTION_COUNTER();

		const FVoxelIntBox LocalBounds = FVoxelIntBox::FromFloatBox_WithPadding(Bounds
			.Scale(1. / double(ChunkSize))
			.IntersectWith(FVoxelBox(MIN_int32, MAX_int32)));

		bool bHasChunks = false;
		Octree.TraverseBounds(LocalBounds, [&](const TVoxelFastOctree<>::FNodeRef& NodeRef)
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
	TVoxelVolumeChunkTreeIterator<ChunkType> CreateIterator(
		const FVoxelVolumeBulkQuery& Query,
		const FVoxelVolumeTransform& StampToQuery,
		const float Scale) const
	{
		checkVoxelSlow(HasChunks(StampToQuery.InverseTransform(Query.GetBounds()).Scale(1. / Scale)));

		return ReinterpretCastRef<TVoxelVolumeChunkTreeIterator<ChunkType>>(FVoxelVolumeChunkTreeIterator::Create(
			ReinterpretCastRef<TVoxelMap<FIntVector, void*>>(KeyToChunk),
			Query,
			StampToQuery,
			Scale));
	}
	TVoxelVolumeChunkTreeIterator<ChunkType> CreateIterator(
		const FVoxelVolumeSparseQuery& Query,
		const FVoxelVolumeTransform& StampToQuery,
		const float Scale) const
	{
		checkVoxelSlow(HasChunks(StampToQuery.InverseTransform(Query.PositionBounds).Scale(1. / Scale)));

		return ReinterpretCastRef<TVoxelVolumeChunkTreeIterator<ChunkType>>(FVoxelVolumeChunkTreeIterator::Create(
			ReinterpretCastRef<TVoxelMap<FIntVector, void*>>(KeyToChunk),
			Query,
			StampToQuery,
			Scale));
	}
	TVoxelVolumeChunkTreeIterator<ChunkType> CreateIterator(const FVoxelDoubleVectorBuffer& Positions) const
	{
		return ReinterpretCastRef<TVoxelVolumeChunkTreeIterator<ChunkType>>(FVoxelVolumeChunkTreeIterator::Create(
			ReinterpretCastRef<TVoxelMap<FIntVector, void*>>(KeyToChunk),
			Positions));
	}

public:
	void Serialize(FArchive& Ar, const int32 Version)
	{
		VOXEL_FUNCTION_COUNTER();

		if (Version < FVoxelVolumeSculptVersion::MergeVersions)
		{
			using FVersion = DECLARE_VOXEL_VERSION
			(
				FirstVersion,
				LegacyVersion,
				RemoveMinMaxKey
			);

			int32 LegacyVersion = FVersion::LatestVersion;
			Ar << LegacyVersion;

			if (LegacyVersion < FVersion::RemoveMinMaxKey)
			{
				Ar << MinKey;
				Ar << MaxKey;
			}
		}

		TVoxelArray<FIntVector> ChunkKeys = KeyToChunk.KeyArray();
		Ar << ChunkKeys;

		if (Ar.IsLoading())
		{
			MinKey = FIntVector(MAX_int32);
			MaxKey = FIntVector(MIN_int32);

			ensure(KeyToChunk.Num() == 0);
			KeyToChunk.Reset();
			KeyToChunk.Reserve(ChunkKeys.Num());

			for (const FIntVector& ChunkKey : ChunkKeys)
			{
				MinKey = FVoxelUtilities::ComponentMin(MinKey, ChunkKey);
				MaxKey = FVoxelUtilities::ComponentMax(MaxKey, ChunkKey);

				KeyToChunk.Add_CheckNew(ChunkKey, new ChunkType());
			}

			for (const FIntVector& ChunkKey : ChunkKeys)
			{
				Octree.TraverseBounds(FVoxelIntBox(ChunkKey), [&](const TVoxelFastOctree<>::FNodeRef& NodeRef)
				{
					if (NodeRef.GetHeight() > 0)
					{
						Octree.CreateAllChildren(NodeRef);
					}
				});
			}
		}

		for (const auto& It : KeyToChunk)
		{
			ConstCast(*It.Value).Serialize(Ar, Version);
		}
	}
	void CopyFrom(const TVoxelVolumeChunkTree& Other)
	{
		VOXEL_FUNCTION_COUNTER();

		MinKey = Other.MinKey;
		MaxKey = Other.MaxKey;
		Octree.CopyFrom(Other.Octree);
		KeyToChunk = Other.KeyToChunk;
	}
	void SetChunk(
		const FIntVector& ChunkKey,
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

			Octree.TraverseBounds(FVoxelIntBox(ChunkKey), [&](const TVoxelFastOctree<>::FNodeRef& NodeRef)
			{
				if (NodeRef.GetHeight() > 0)
				{
					Octree.CreateAllChildren(NodeRef);
				}
			});

			KeyToChunk.Add_EnsureNew(ChunkKey, Chunk);
		}
	}

private:
	FIntVector MinKey = FIntVector(MAX_int32);
	FIntVector MaxKey = FIntVector(MIN_int32);
	TVoxelFastOctree<> Octree{ 30 };
	TVoxelMap<FIntVector, TVoxelRefCountPtr<const ChunkType>> KeyToChunk;
};