// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelVolumeLayer.h"
#include "VoxelVolumeStampWrapper.h"
#include "VoxelHeightLayer.h"

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelVolumeLayer);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelOptionalBox FVoxelVolumeLayer::GetBoundsToGenerate(FVoxelDependencyCollector& DependencyCollector) const
{
	for (int32 LOD = 0; LOD < LODToTree.Num(); LOD++)
	{
		const FVoxelStampTree& Tree = GetTree(LOD);

		DependencyCollector.AddDependency(*Tree.Dependency, FVoxelBox::Infinite);
	}

	FVoxelOptionalBox Bounds;
	if (PreviousHeightLayer)
	{
		check(!PreviousVolumeLayer);
		Bounds = PreviousHeightLayer->GetBoundsToGenerate(DependencyCollector);
	}
	else if (PreviousVolumeLayer)
	{
		Bounds = PreviousVolumeLayer->GetBoundsToGenerate(DependencyCollector);
	}

	if (Bounds.IsValid())
	{
		Bounds = Bounds->IntersectWith(IntersectBounds);
	}

	for (int32 LOD = 0; LOD < LODToTree.Num(); LOD++)
	{
		const FVoxelStampTree& Tree = GetTree(LOD);

		if (!Tree.AABBTree.IsEmpty())
		{
			// Due to float imprecision this might be smaller than the true bounds
			Bounds += Tree.AABBTree.GetBounds().GetBox();
		}
	}

	return Bounds;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelVolumeLayer::HasStamps(
	const FVoxelQuery& Query,
	const FVoxelBox& Bounds) const
{
	if (!IntersectBounds.Intersects(Bounds))
	{
		return false;
	}

	if (PreviousHeightLayer &&
		PreviousHeightLayer->HasStamps(Query, Bounds.IntersectWith(IntersectBounds), false))
	{
		return true;
	}

	if (PreviousVolumeLayer &&
		PreviousVolumeLayer->HasStamps(Query, Bounds.IntersectWith(IntersectBounds)))
	{
		return true;
	}

	const FVoxelStampTree& Tree = GetTree(Query.LOD);
	Query.DependencyCollector.AddDependency(*Tree.Dependency, Bounds);
	return Tree.AABBTree.Intersects(FVoxelFastBox(Bounds));
}

bool FVoxelVolumeLayer::HasVolumeStamps(
	const FVoxelQuery& Query,
	const FVoxelBox& Bounds) const
{
	if (!IntersectBounds.Intersects(Bounds))
	{
		return false;
	}

	if (PreviousVolumeLayer &&
		PreviousVolumeLayer->HasVolumeStamps(Query, Bounds.IntersectWith(IntersectBounds)))
	{
		return true;
	}

	const FVoxelStampTree& Tree = GetTree(Query.LOD);
	Query.DependencyCollector.AddDependency(*Tree.Dependency, Bounds);
	return Tree.AABBTree.Intersects(FVoxelFastBox(Bounds));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelVolumeLayer::Sample(
	const FVoxelQueryCache& Cache,
	const FVoxelVolumeBulkQuery& Query) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Query.Num());

	const TUniquePtr<FVoxelStampTree::FIterator> Iterator = GetTree(Query.Query.LOD).CreateIterator(
		Query.Query,
		Query.GetBounds());

	SampleStamps(
		Cache,
		Query.Query,
		Query.WithQuery(Query.Query.MakeChild_Layer(WeakLayer)),
		Iterator->Stamps);
}

void FVoxelVolumeLayer::Sample(
	const FVoxelQueryCache& Cache,
	const FVoxelVolumeSparseQuery& Query) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Query.Num());

	const TUniquePtr<FVoxelStampTree::FIterator> Iterator = GetTree(Query.Query.LOD).CreateIterator(
		Query.Query,
		Query.PositionBounds);

	SampleStamps(
		Cache,
		Query.Query,
		Query.WithQuery(Query.Query.MakeChild_Layer(WeakLayer)),
		Iterator->Stamps);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename QueryType>
void FVoxelVolumeLayer::SampleStamps(
	const FVoxelQueryCache& Cache,
	const FVoxelQuery& PreviousQuery,
	const QueryType& Query,
	const TConstVoxelArrayView<FVoxelStampTree::FStamp> Stamps) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Query.Num());

	for (int32 StampIndex = Stamps.Num() - 1; StampIndex >= 0; StampIndex--)
	{
		const FVoxelStampTree::FStamp& Stamp = Stamps[StampIndex];
		const FVoxelVolumeStampRuntime& TypedStamp = Stamp.GetStamp<FVoxelVolumeStampRuntime>();

		if (!TypedStamp.ShouldUseQueryPrevious())
		{
			continue;
		}

		TVoxelInlineArray<QueryType, 1> LocalQueries;
		LocalQueries.Add(Query);

		for (const FVoxelStampTreeElement* Element : Stamp.GetElements())
		{
			TVoxelInlineArray<QueryType, 1> NextQueries;
			for (const QueryType& LocalQuery : LocalQueries)
			{
				TVoxelOptional<QueryType> Inside;
				LocalQuery.Split(
					Element->Bounds,
					Inside,
					NextQueries);

				if (!Inside)
				{
					continue;
				}

				const FVoxelVolumeQueryPrevious QueryPrevious([&](const auto& InQuery)
				{
					VOXEL_SCOPE_COUNTER_NUM("Query previous", InQuery.Num());

					this->SampleStamps(
						// Make sure to not reuse the cache
						{},
						InQuery.Query,
						InQuery,
						Stamps.LeftOf(StampIndex));
				});

				Inside->QueryPrevious = &QueryPrevious;

				FVoxelVolumeStampWrapper::Apply(
					WeakLayer,
					TypedStamp,
					*Inside,
					Element->VolumeStampToQuery);
			}
			LocalQueries = MoveTemp(NextQueries);
		}

		// Sample all the stamps before us, but only outside the override bounds
		for (const QueryType& LocalQuery : LocalQueries)
		{
			this->SampleStamps(
				Cache,
				PreviousQuery,
				LocalQuery,
				Stamps.LeftOf(StampIndex));
		}

		// Sample all the stamps after us everywhere
		if (StampIndex + 1 < Stamps.Num())
		{
			this->SampleStampsImpl(
				Query,
				Stamps.RightOf(StampIndex + 1));
		}

		return;
	}

	if (const TVoxelOptional<QueryType> ShrunkQuery = Query.ShrinkTo(IntersectBounds))
	{
		if (PreviousHeightLayer)
		{
			check(!PreviousVolumeLayer);

			PreviousHeightLayer->SampleAsVolume(
				Cache,
				ShrunkQuery->WithQuery(PreviousQuery));
		}
		else if (PreviousVolumeLayer)
		{
			PreviousVolumeLayer->Sample(
				Cache,
				ShrunkQuery->WithQuery(PreviousQuery));
		}
	}

	this->SampleStampsImpl(Query, Stamps);
}

template<typename QueryType>
void FVoxelVolumeLayer::SampleStampsImpl(
	const QueryType& Query,
	const TConstVoxelArrayView<FVoxelStampTree::FStamp> Stamps) const
{
	for (const FVoxelStampTree::FStamp& Stamp : Stamps)
	{
		checkVoxelSlow(!Stamp.GetStamp<FVoxelVolumeStampRuntime>().ShouldUseQueryPrevious());

		if (const FVoxelStampTreeElement* Element = Stamp.GetUniqueElement())
		{
			if (const TVoxelOptional<QueryType> ElementQuery = Query.ShrinkTo(Element->Bounds))
			{
				FVoxelVolumeStampWrapper::Apply(
					WeakLayer,
					Stamp.GetStamp<FVoxelVolumeStampRuntime>(),
					*ElementQuery,
					Element->VolumeStampToQuery);
			}

			continue;
		}

		TVoxelInlineArray<QueryType, 1> LocalQueries;
		LocalQueries.Add(Query);

		for (const FVoxelStampTreeElement* Element : Stamp.GetElements())
		{
			TVoxelInlineArray<QueryType, 1> NextQueries;
			for (const QueryType& LocalQuery : LocalQueries)
			{
				TVoxelOptional<QueryType> Inside;
				LocalQuery.Split(
					Element->Bounds,
					Inside,
					NextQueries);

				if (!Inside)
				{
					continue;
				}

				FVoxelVolumeStampWrapper::Apply(
					WeakLayer,
					Stamp.GetStamp<FVoxelVolumeStampRuntime>(),
					*Inside,
					Element->VolumeStampToQuery);
			}

			if (NextQueries.Num() == 0)
			{
				break;
			}

			LocalQueries = MoveTemp(NextQueries);
		}
	}
}