// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMetadataOverrides.h"
#include "Sculpt/VoxelToolBrush.h"
#include "Sculpt/VoxelSculptMode.h"
#include "Sculpt/Volume/VoxelVolumeModifier.h"
#include "VoxelPaintVolumeModifier.generated.h"

class FVoxelToolRuntimeBrush;

USTRUCT()
struct VOXEL_API FVoxelPaintVolumeModifier : public FVoxelVolumeModifier
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	UPROPERTY()
	FVector Center = FVector(ForceInit);

	UPROPERTY()
	TObjectPtr<UVoxelSurfaceTypeInterface> SurfaceTypeToPaint;

	UPROPERTY()
	FVoxelMetadataOverrides MetadatasToPaint;

	UPROPERTY()
	float Radius = 0.f;

	UPROPERTY()
	float Strength = 0.f;

	UPROPERTY()
	EVoxelSculptMode Mode = {};

	UPROPERTY()
	FVoxelToolBrush Brush;

public:
	//~ Begin FVoxelVolumeTransaction_Modifier Interface
	virtual void Initialize_GameThread() override;
	virtual FVoxelBox GetBounds() const override;

	virtual void GetUsage(
		bool& bWritesDistances,
		bool& bWritesSurfaceTypes,
		TVoxelSet<FVoxelMetadataRef>& MetadataRefsToWrite) const override;

	virtual void Apply(const FData& Data) const override;
	//~ End FVoxelVolumeTransaction_Modifier Interface

private:
	FVoxelSurfaceType RuntimeSurfaceType;
	TSharedPtr<FVoxelRuntimeMetadataOverrides> RuntimeMetadatas;

	TSharedPtr<const FVoxelToolRuntimeBrush> RuntimeBrush;
};