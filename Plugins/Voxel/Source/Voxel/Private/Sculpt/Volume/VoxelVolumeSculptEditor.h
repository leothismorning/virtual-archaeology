// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelStackLayer.h"
#include "VoxelVolumeBlendMode.h"
#include "Sculpt/Volume/VoxelVolumeModifier.h"
#include "Sculpt/Volume/VoxelVolumeSculptData.h"
#include "Sculpt/Volume/VoxelVolumeSculptCache.h"
#include "Sculpt/Volume/VoxelVolumeSculptDefinitions.h"

class FVoxelVolumeSculptEditor : FVoxelVolumeSculptDefinitions
{
public:
	const bool bEnableDiffing;
	const FMatrix SculptToWorld;
	const FVoxelVolumeSculptDataId SculptDataId;
	const float VoxelSize;
	const EVoxelVolumeBlendMode BlendMode;
	const TSharedRef<FVoxelLayers> Layers;
	const TSharedRef<FVoxelSurfaceTypeTable> SurfaceTypeTable;
	const FVoxelWeakStackLayer WeakLayer;
	const TSharedRef<FVoxelVolumeSculptCache> Cache;
	const TSharedRef<const FVoxelVolumeModifier> Modifier;

	FVoxelVolumeSculptEditor(
		const bool bEnableDiffing,
		const FMatrix& SculptToWorld,
		const FVoxelVolumeSculptDataId SculptDataId,
		const EVoxelVolumeBlendMode BlendMode,
		const TSharedRef<FVoxelLayers>& Layers,
		const TSharedRef<FVoxelSurfaceTypeTable>& SurfaceTypeTable,
		const FVoxelWeakStackLayer& WeakLayer,
		const TSharedRef<FVoxelVolumeSculptCache>& Cache,
		const TSharedRef<const FVoxelVolumeModifier>& Modifier)
		: bEnableDiffing(bEnableDiffing)
		, SculptToWorld(SculptToWorld)
		, SculptDataId(SculptDataId)
		, VoxelSize(SculptToWorld.GetScaleVector().GetAbsMax())
		, BlendMode(BlendMode)
		, Layers(Layers)
		, SurfaceTypeTable(SurfaceTypeTable)
		, WeakLayer(WeakLayer)
		, Cache(Cache)
		, Modifier(Modifier)
	{
	}

public:
	FVoxelBox DoWork(FVoxelVolumeSculptInnerData& SculptData);

private:
	bool bWritesDistances = false;
	bool bWritesSurfaceTypes = false;
	TVoxelSet<FVoxelMetadataRef> MetadataRefsToWrite;

	FIntVector Size = FIntVector(ForceInit);
	FVoxelIntBox BoundsToSculpt;
	FIntVector ChunkKeyOffset = FIntVector(ForceInit);
	FVoxelIntBox RelativeChunkKeyBounds;
	FVoxelIntBox RelativeChunkKeyBoundsToEdit;

	TVoxelArray<float> ClosestX;
	TVoxelArray<float> ClosestY;
	TVoxelArray<float> ClosestZ;

	TVoxelArray<float> Distances;
	TVoxelArray<float> PreviousDistances;

	void ApplyModifier(FVoxelVolumeSculptInnerData& SculptData);
	void Propagate(FVoxelVolumeSculptInnerData& SculptData);
};