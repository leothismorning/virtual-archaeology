// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelQuery.h"
#include "VoxelAABBTree.h"
#include "VoxelVolumeStamp.h"
#include "VoxelStampTransform.h"

struct FVoxelStampTreeElement
{
	const FVoxelStampRuntime* Stamp = nullptr;
	int32 ChildIndex = -1;

	union
	{
		FVoxelHeightTransform HeightStampToQuery;
		FVoxelVolumeTransform VolumeStampToQuery;
	};

	// in Query space
	FVoxelBox Bounds;

	FVoxelStampTreeElement()
	{
		FMemory::Memzero(HeightStampToQuery);
		FMemory::Memzero(VolumeStampToQuery);
	}
};
checkStatic(sizeof(FVoxelStampTreeElement) == 128);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct FVoxelStampTree
{
public:
	const TSharedRef<FVoxelDependency3D> Dependency;
	const FVoxelAABBTree AABBTree;
	// Deterministically sorted
	const TVoxelArray<FVoxelStampTreeElement> Elements;
	const TVoxelArray<TSharedRef<const FVoxelStampRuntime>> Stamps;
	const TSharedPtr<FVoxelStampTree> PreviousTree;

	FVoxelStampTree(
		const TSharedRef<FVoxelDependency3D>& Dependency,
		FVoxelAABBTree&& AABBTree,
		TVoxelArray<FVoxelStampTreeElement>&& Elements,
		TVoxelArray<TSharedRef<const FVoxelStampRuntime>>&& Stamps,
		const TSharedPtr<FVoxelStampTree>& PreviousTree)
		: Dependency(Dependency)
		, AABBTree(MoveTemp(AABBTree))
		, Elements(MoveTemp(Elements))
		, Stamps(MoveTemp(Stamps))
		, PreviousTree(PreviousTree)
	{
	}

public:
	struct FStamp
	{
	public:
		template<typename T>
		FORCEINLINE const T& GetStamp() const
		{
			checkVoxelSlow(Stamp);
			return CastStructChecked<T>(*Stamp);
		}
		FORCEINLINE const FVoxelStampTreeElement* GetUniqueElement() const
		{
			if (Elements.Num() > 1)
			{
				return nullptr;
			}

			return Elements[0];
		}
		FORCEINLINE TConstVoxelArrayView<const FVoxelStampTreeElement*> GetElements() const
		{
			return Elements;
		}

	private:
		const FVoxelStampRuntime* Stamp = nullptr;
		TConstVoxelArrayView<const FVoxelStampTreeElement*> Elements;

		FStamp(
			const FVoxelStampRuntime* Stamp,
			const TConstVoxelArrayView<const FVoxelStampTreeElement*> Elements)
			: Stamp(Stamp)
			, Elements(Elements)
		{
			checkVoxelSlow(Elements.Num() > 0);

			for (const FVoxelStampTreeElement* Element : Elements)
			{
				checkVoxelSlow(Element->Stamp == Stamp);
			}
		}

		friend FVoxelStampTree;
	};
	struct FIterator
	{
	public:
		TVoxelInlineArray<FStamp, 128> Stamps;

	private:
		TVoxelInlineArray<const FVoxelStampTreeElement*, 128> ElementStorage;

		friend FVoxelStampTree;
	};
	TUniquePtr<FIterator> CreateIterator(
		const FVoxelQuery& Query,
		const FVoxelBox& Bounds) const;

public:
	template<typename LambdaType>
	void ForeachElement_Unsorted(
		const FVoxelQuery& Query,
		const FVoxelBox& Bounds,
		LambdaType Lambda) const
	{
		VOXEL_FUNCTION_COUNTER();

		Query.DependencyCollector.AddDependency(*Dependency, Bounds);

		AABBTree.TraverseBounds(FVoxelFastBox(Bounds), [&](const int32 Index)
		{
			Lambda(Elements[Index]);
		});
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class FVoxelStampTreeManager : public FVoxelSingleton
{
public:
	void AddTask(TVoxelUniqueFunction<void()> Lambda);
	void ProcessTask();

private:
	FVoxelCriticalSection CriticalSection;
	TQueue<TVoxelUniqueFunction<void()>> Tasks_RequiresLock;
};
extern FVoxelStampTreeManager* GVoxelStampTreeManager;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct FVoxelFutureStampTree
{
public:
	FVoxelFutureStampTree() = default;
	UE_NONCOPYABLE(FVoxelFutureStampTree);

	FORCEINLINE FVoxelStampTree& GetTree() const
	{
		if (PrivateIsComplete.Get())
		{
			return *Tree;
		}

		return GetTree_Impl();
	}
	FORCEINLINE bool IsComplete() const
	{
		return PrivateIsComplete.Get();
	}

	void Set(const TSharedRef<FVoxelStampTree>& NewTree);
	void Then(TVoxelUniqueFunction<void(const TSharedRef<FVoxelStampTree>&)> Lambda);

private:
	FVoxelCriticalSection_NoPadding CriticalSection;
	TSharedPtr<FVoxelStampTree> Tree;
	TVoxelAtomic<bool> PrivateIsComplete;
	TVoxelArray<TVoxelUniqueFunction<void(const TSharedRef<FVoxelStampTree>&)>> Continuations_RequiresLock;

	FVoxelStampTree& GetTree_Impl() const;
};