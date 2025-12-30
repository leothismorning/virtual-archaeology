// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLayerBase.h"

class FVoxelHeightLayer;

class VOXEL_API FVoxelVolumeLayer : public FVoxelLayerBase
{
public:
	const FVoxelBox IntersectBounds;
	const TSharedPtr<const FVoxelHeightLayer> PreviousHeightLayer;
	const TSharedPtr<const FVoxelVolumeLayer> PreviousVolumeLayer;

	VOXEL_COUNT_INSTANCES()

	FVoxelVolumeLayer(
		const FVoxelWeakStackLayer& WeakLayer,
		const TVoxelArray<TSharedRef<FVoxelFutureStampTree>>& LODToTree,
		const FVoxelBox& IntersectBounds,
		const TSharedPtr<const FVoxelHeightLayer>& PreviousHeightLayer,
		const TSharedPtr<const FVoxelVolumeLayer>& PreviousVolumeLayer)
		: FVoxelLayerBase(WeakLayer, LODToTree)
		, IntersectBounds(IntersectBounds)
		, PreviousHeightLayer(PreviousHeightLayer)
		, PreviousVolumeLayer(PreviousVolumeLayer)
	{
	}

public:
	FVoxelOptionalBox GetBoundsToGenerate(FVoxelDependencyCollector& DependencyCollector) const;

public:
	bool HasStamps(
		const FVoxelQuery& Query,
		const FVoxelBox& Bounds) const;

	bool HasVolumeStamps(
		const FVoxelQuery& Query,
		const FVoxelBox& Bounds) const;

public:
	void Sample(
		const FVoxelQueryCache& Cache,
		const FVoxelVolumeBulkQuery& Query) const;

	void Sample(
		const FVoxelQueryCache& Cache,
		const FVoxelVolumeSparseQuery& Query) const;

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
};