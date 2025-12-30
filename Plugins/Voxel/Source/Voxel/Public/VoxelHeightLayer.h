// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLayerBase.h"

class VOXEL_API FVoxelHeightLayer : public FVoxelLayerBase
{
public:
	const float MaxDistance;
	const TSharedPtr<const FVoxelHeightLayer> PreviousLayer;

	VOXEL_COUNT_INSTANCES()

	FVoxelHeightLayer(
		const FVoxelWeakStackLayer& WeakLayer,
		const float MaxDistance,
		const TVoxelArray<TSharedRef<FVoxelFutureStampTree>>& LODToTree,
		const TSharedPtr<const FVoxelHeightLayer>& PreviousLayer)
		: FVoxelLayerBase(WeakLayer, LODToTree)
		, MaxDistance(MaxDistance)
		, PreviousLayer(PreviousLayer)
	{
	}

public:
	FVoxelOptionalBox GetBoundsToGenerate(FVoxelDependencyCollector& DependencyCollector) const;

public:
	bool HasStamps(
		const FVoxelQuery& Query,
		const FVoxelBox& Bounds,
		bool bExtendByMaxDistance) const;

public:
	void Sample(
		const FVoxelQueryCache& Cache,
		const FVoxelHeightBulkQuery& Query) const;

	void Sample(
		const FVoxelQueryCache& Cache,
		const FVoxelHeightSparseQuery& Query) const;

private:
	bool SampleCached(
		const FVoxelQueryCache& Cache,
		const FVoxelHeightBulkQuery& Query) const;

private:
	template<typename QueryType>
	void SampleStamps(
		const FVoxelQueryCache& Cache,
		const FVoxelQuery& PreviousQuery,
		const QueryType& Query,
		TConstVoxelArrayView<FVoxelStampTree::FStamp> Stamps) const;

	template<typename QueryType>
	void SampleStampsImpl(
		const QueryType& Query,
		TConstVoxelArrayView<FVoxelStampTree::FStamp> Stamps) const;

public:
	void SampleAsVolume(
		const FVoxelQueryCache& Cache,
		const FVoxelVolumeBulkQuery& Query) const;

	void SampleAsVolume(
		const FVoxelQueryCache& Cache,
		const FVoxelVolumeSparseQuery& Query) const;
};