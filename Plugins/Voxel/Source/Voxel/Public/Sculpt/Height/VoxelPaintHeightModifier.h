// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMetadataOverrides.h"
#include "Sculpt/VoxelToolBrush.h"
#include "Sculpt/VoxelSculptMode.h"
#include "Sculpt/Height/VoxelHeightModifier.h"
#include "VoxelPaintHeightModifier.generated.h"

class FVoxelToolRuntimeBrush;

USTRUCT()
struct VOXEL_API FVoxelPaintHeightModifier : public FVoxelHeightModifier
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	UPROPERTY()
	FVector2D Center = FVector2D(ForceInit);

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
	//~ Begin FVoxelHeightTransaction_Modifier Interface
	virtual void Initialize_GameThread() override;
	virtual FVoxelBox2D GetBounds() const override;

	virtual void GetUsage(
		bool& bWritesDistances,
		bool& bWritesSurfaceTypes,
		TVoxelSet<FVoxelMetadataRef>& MetadataRefsToWrite) const override;

	virtual void Apply(const FData& Data) const override;
	//~ End FVoxelHeightTransaction_Modifier Interface

private:
	FVoxelSurfaceType RuntimeSurfaceType;
	TSharedPtr<FVoxelRuntimeMetadataOverrides> RuntimeMetadatas;

	TSharedPtr<const FVoxelToolRuntimeBrush> RuntimeBrush;
};