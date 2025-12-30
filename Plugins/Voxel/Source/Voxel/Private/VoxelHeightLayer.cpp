// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelHeightLayer.h"
#include "VoxelHeightStamp.h"
#include "VoxelHeightStampWrapper.h"
#include "VoxelHeightLayerImpl.ispc.generated.h"
#include "Buffer/VoxelBaseBuffers.h"

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelHeightLayer);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelOptionalBox FVoxelHeightLayer::GetBoundsToGenerate(FVoxelDependencyCollector& DependencyCollector) const
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelOptionalBox Bounds;
	if (PreviousLayer)
	{
		Bounds = PreviousLayer->GetBoundsToGenerate(DependencyCollector);
	}

	for (int32 LOD = 0; LOD < LODToTree.Num(); LOD++)
	{
		const FVoxelStampTree& Tree = GetTree(LOD);

		DependencyCollector.AddDependency(*Tree.Dependency, FVoxelBox::Infinite);

		if (!Tree.AABBTree.IsEmpty())
		{
			Bounds += Tree.AABBTree.GetBounds().GetBox();
		}
	}

	return Bounds;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelHeightLayer::HasStamps(
	const FVoxelQuery& Query,
	const FVoxelBox& Bounds,
	const bool bExtendByMaxDistance) const
{
	VOXEL_FUNCTION_COUNTER();

	if (PreviousLayer &&
		PreviousLayer->HasStamps(Query, Bounds, bExtendByMaxDistance))
	{
		return true;
	}

	FVoxelInterval RangeZ = FVoxelInterval::InvertedInfinite;

	GetTree(Query.LOD).ForeachElement_Unsorted(
		Query,
		Bounds.Extend(FVector(0, 0, bExtendByMaxDistance ? MaxDistance : 0.f)),
		[&](const FVoxelStampTreeElement& Element)
		{
			RangeZ += Element.Bounds.GetZ();
		});

	if (!RangeZ.IsValid())
	{
		return false;
	}

	return Bounds.GetZ().Intersects(RangeZ.Extend(bExtendByMaxDistance ? MaxDistance : 0.f));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelHeightLayer::Sample(
	const FVoxelQueryCache& Cache,
	const FVoxelHeightBulkQuery& Query) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Query.Num());

	if (SampleCached(Cache, Query))
	{
		return;
	}

	const TUniquePtr<FVoxelStampTree::FIterator> Iterator = GetTree(Query.Query.LOD).CreateIterator(
		Query.Query,
		Query.GetBounds().ToBox3D_Infinite());

	SampleStamps(
		Cache,
		Query.Query,
		Query.WithQuery(Query.Query.MakeChild_Layer(WeakLayer)),
		Iterator->Stamps);
}

void FVoxelHeightLayer::Sample(
	const FVoxelQueryCache& Cache,
	const FVoxelHeightSparseQuery& Query) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Query.Num());
	ensure(Cache.HeightLayerToEntry.Num() == 0);

	const TUniquePtr<FVoxelStampTree::FIterator> Iterator = GetTree(Query.Query.LOD).CreateIterator(
		Query.Query,
		Query.PositionBounds.ToBox3D_Infinite());

	SampleStamps(
		Cache,
		Query.Query,
		Query.WithQuery(Query.Query.MakeChild_Layer(WeakLayer)),
		Iterator->Stamps);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelHeightLayer::SampleCached(
	const FVoxelQueryCache& Cache,
	const FVoxelHeightBulkQuery& Query) const
{
	const FVoxelQueryCache::FEntry* Entry = Cache.HeightLayerToEntry.Find(WeakLayer);
	if (!Entry)
	{
		return false;
	}

	if (Entry->Heights.Num() != Query.Num())
	{
		// Intersect stamp shrunk the query, we can't use cache
		return false;
	}

	if (VOXEL_DEBUG)
	{
		Sample({}, Query);

		ensure(FVoxelUtilities::Equal(Query.Heights, Entry->Heights) || FVoxelShouldCancel());
	}

	Query.Query.DependencyCollector.AddDependencies(*Entry->DependencyCollector);

	FVoxelUtilities::Memcpy(Query.Heights, Entry->Heights);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename QueryType>
void FVoxelHeightLayer::SampleStamps(
	const FVoxelQueryCache& Cache,
	const FVoxelQuery& PreviousQuery,
	const QueryType& Query,
	const TConstVoxelArrayView<FVoxelStampTree::FStamp> Stamps) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Query.Num());

	for (int32 StampIndex = Stamps.Num() - 1; StampIndex >= 0; StampIndex--)
	{
		const FVoxelStampTree::FStamp& Stamp = Stamps[StampIndex];
		const FVoxelHeightStampRuntime& TypedStamp = Stamp.GetStamp<FVoxelHeightStampRuntime>();

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
					FVoxelBox2D(Element->Bounds),
					Inside,
					NextQueries);

				if (!Inside)
				{
					continue;
				}

				const FVoxelHeightQueryPrevious QueryPrevious([&](const auto& InQuery)
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

				FVoxelHeightStampWrapper::Apply(
					WeakLayer,
					TypedStamp,
					*Inside,
					Element->HeightStampToQuery);
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

	if (PreviousLayer)
	{
		PreviousLayer->Sample(
			Cache,
			Query.WithQuery(PreviousQuery));
	}

	this->SampleStampsImpl(Query, Stamps);
}

template<typename QueryType>
void FVoxelHeightLayer::SampleStampsImpl(
	const QueryType& Query,
	const TConstVoxelArrayView<FVoxelStampTree::FStamp> Stamps) const
{
	for (const FVoxelStampTree::FStamp& Stamp : Stamps)
	{
		checkVoxelSlow(!Stamp.GetStamp<FVoxelHeightStampRuntime>().ShouldUseQueryPrevious());

		if (const FVoxelStampTreeElement* Element = Stamp.GetUniqueElement())
		{
			if (const TVoxelOptional<QueryType> ElementQuery = Query.ShrinkTo(FVoxelBox2D(Element->Bounds)))
			{
				FVoxelHeightStampWrapper::Apply(
					WeakLayer,
					Stamp.GetStamp<FVoxelHeightStampRuntime>(),
					*ElementQuery,
					Element->HeightStampToQuery);
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
					FVoxelBox2D(Element->Bounds),
					Inside,
					NextQueries);

				if (!Inside)
				{
					continue;
				}

				FVoxelHeightStampWrapper::Apply(
					WeakLayer,
					Stamp.GetStamp<FVoxelHeightStampRuntime>(),
					*Inside,
					Element->HeightStampToQuery);
			}

			if (NextQueries.Num() == 0)
			{
				break;
			}

			LocalQueries = MoveTemp(NextQueries);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelHeightLayer::SampleAsVolume(
	const FVoxelQueryCache& Cache,
	const FVoxelVolumeBulkQuery& Query) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Query.Num());

	const FIntPoint Size2D = FVoxelIntBox2D(Query.Indices).Size();
	const FVector2d Start2D = FVector2d(Query.Start + FVector(Query.Indices.Min) * Query.Step);

	if (!HasStamps(Query.Query, Query.GetBounds(), true))
	{
		if (VOXEL_DEBUG)
		{
			FVoxelFloatBuffer Heights;
			Heights.Allocate(Size2D.X * Size2D.Y);
			Heights.SetAll(FVoxelUtilities::NaNf());

			Sample(
				{},
				FVoxelHeightBulkQuery::Create(
					Query.Query,
					Heights.View(),
					Start2D,
					Size2D,
					Query.Step));

			const FVoxelBox Bounds = Query.GetBounds();

			for (const float Height : Heights)
			{
				if (FVoxelUtilities::IsNaN(Height))
				{
					continue;
				}

				ensure(FMath::Abs(Bounds.Min.Z - Height) > MaxDistance);
				ensure(FMath::Abs(Bounds.Max.Z - Height) > MaxDistance);
			}
		}

		return;
	}

	FVoxelFloatBuffer Heights;
	Heights.Allocate(Size2D.X * Size2D.Y);
	Heights.SetAll(FVoxelUtilities::NaNf());

	Sample(
		Cache,
		FVoxelHeightBulkQuery::Create(
			Query.Query,
			Heights.View(),
			Start2D,
			Size2D,
			Query.Step));

	VOXEL_SCOPE_COUNTER_NUM("HeightToVolume_Dense", Query.Num());

	ispc::VoxelHeightLayer_HeightToVolume_Dense(
		Query.ISPC(),
		Heights.GetData(),
		MaxDistance);
}

void FVoxelHeightLayer::SampleAsVolume(
	const FVoxelQueryCache& Cache,
	const FVoxelVolumeSparseQuery& Query) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Query.Num());

	if (!HasStamps(Query.Query, Query.PositionBounds, true))
	{
		if (VOXEL_DEBUG)
		{
			FVoxelFloatBuffer Heights;
			Heights.Allocate(Query.Num());
			Heights.SetAll(FVoxelUtilities::NaNf());

			FVoxelDoubleVector2DBuffer Positions2D;
			Positions2D.X = Query.Positions.X;
			Positions2D.Y = Query.Positions.Y;

			Sample(
				{},
				FVoxelHeightSparseQuery::Create(
					Query.Query,
					Heights.View(),
					{},
					{},
					Positions2D,
					false,
					{}));

			for (const float Height : Heights)
			{
				if (FVoxelUtilities::IsNaN(Height))
				{
					continue;
				}

				ensure(FMath::Abs(Query.PositionBounds.Min.Z - Height) > MaxDistance);
				ensure(FMath::Abs(Query.PositionBounds.Max.Z - Height) > MaxDistance);
			}
		}

		return;
	}

	FVoxelFloatBuffer IndirectHeights;
	IndirectHeights.Allocate(Query.IndirectDistances.Num());
	IndirectHeights.SetAll(FVoxelUtilities::NaNf());

	FVoxelDoubleVector2DBuffer Positions2D;
	Positions2D.X = Query.Positions.X;
	Positions2D.Y = Query.Positions.Y;

	ensure(!Query.QueryPrevious);

	const FVoxelHeightSparseQuery SparseQuery2D
	{
		Query.Query,
		Query.IndirectSurfaceTypes,
		Query.IndirectMetadata,
		Query.Indirection,
		Query.bQuerySurfaceTypes,
		Query.MetadatasToQuery,
		IndirectHeights.View(),
		Positions2D,
		FVoxelBox2D(Query.PositionBounds),
		{}
	};

	Sample(
		Cache,
		SparseQuery2D);

	VOXEL_SCOPE_COUNTER_NUM("HeightToVolume_Sparse", Query.Num());

	ispc::VoxelHeightLayer_HeightToVolume_Sparse(
		Query.ISPC(),
		IndirectHeights.GetData(),
		MaxDistance);
}