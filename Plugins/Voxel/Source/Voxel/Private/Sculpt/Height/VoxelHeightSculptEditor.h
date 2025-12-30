// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelStackLayer.h"
#include "VoxelHeightBlendMode.h"
#include "Sculpt/Height/VoxelHeightModifier.h"
#include "Sculpt/Height/VoxelHeightSculptData.h"
#include "Sculpt/Height/VoxelHeightSculptCache.h"
#include "Sculpt/Height/VoxelHeightSculptDefinitions.h"

class FVoxelHeightSculptEditor : FVoxelHeightSculptDefinitions
{
public:
	const bool bRelativeHeight;
	const FTransform2d SculptToWorld;
	const FVoxelHeightSculptDataId SculptDataId;
	const float ScaleZ;
	const float OffsetZ;
	const EVoxelHeightBlendMode BlendMode;
	const TSharedRef<FVoxelLayers> Layers;
	const TSharedRef<FVoxelSurfaceTypeTable> SurfaceTypeTable;
	const FVoxelWeakStackLayer WeakLayer;
	const TSharedRef<FVoxelHeightSculptCache> Cache;
	const TSharedRef<const FVoxelHeightModifier> Modifier;

	FVoxelHeightSculptEditor(
		const bool bRelativeHeight,
		const FTransform2d& SculptToWorld,
		const float ScaleZ,
		const float OffsetZ,
		const FVoxelHeightSculptDataId SculptDataId,
		const EVoxelHeightBlendMode BlendMode,
		const TSharedRef<FVoxelLayers>& Layers,
		const TSharedRef<FVoxelSurfaceTypeTable>& SurfaceTypeTable,
		const FVoxelWeakStackLayer& WeakLayer,
		const TSharedRef<FVoxelHeightSculptCache>& Cache,
		const TSharedRef<const FVoxelHeightModifier>& Modifier)
		: bRelativeHeight(bRelativeHeight)
		, SculptToWorld(SculptToWorld)
		, SculptDataId(SculptDataId)
		, ScaleZ(ScaleZ)
		, OffsetZ(OffsetZ)
		, BlendMode(BlendMode)
		, Layers(Layers)
		, SurfaceTypeTable(SurfaceTypeTable)
		, WeakLayer(WeakLayer)
		, Cache(Cache)
		, Modifier(Modifier)
	{
	}

public:
	FVoxelBox2D DoWork(FVoxelHeightSculptInnerData& SculptData);

private:
	bool bWritesHeights = false;
	bool bWritesSurfaceTypes = false;
	TVoxelSet<FVoxelMetadataRef> MetadataRefsToWrite;

	FIntPoint Size = FIntPoint(ForceInit);
	FVoxelIntBox2D BoundsToSculpt;
	FIntPoint ChunkKeyOffset = FIntPoint(ForceInit);
	FVoxelIntBox2D RelativeChunkKeyBounds;
	FVoxelIntBox2D RelativeChunkKeyBoundsToEdit;

	TVoxelArray<float> ClosestX;
	TVoxelArray<float> ClosestY;
	TVoxelArray<float> ClosestZ;

	TVoxelArray<float> Heights;
	TVoxelArray<float> PreviousHeights;

	void ApplyModifier(FVoxelHeightSculptInnerData& SculptData);
};