// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLayer.h"
#include "VoxelStamp.h"
#include "VoxelStampQuery.h"
#include "VoxelStampRuntime.h"
#include "VoxelStampTransform.h"
#include "VoxelVolumeBlendMode.h"
#include "VoxelVolumeStamp.generated.h"

class FVoxelQuery;
class FVoxelMetadataView;

USTRUCT(meta = (Abstract))
struct VOXEL_API FVoxelVolumeStamp : public FVoxelStamp
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	// Layer that this stamps belong to
	// You can control the order of layers in Layer Stacks
	// You can select the layer stack to use in your Voxel World or PCG Sampler settings
	UPROPERTY(EditAnywhere, Category = "Config", meta = (EditCondition = "!bDisableEditingLayers", HideEditConditionToggle))
	TObjectPtr<UVoxelVolumeLayer> Layer;

	UPROPERTY()
	TArray<TObjectPtr<UVoxelVolumeLayer>> VoxelLayers;

	UPROPERTY(EditAnywhere, Category = "Config", meta = (EditCondition = "!bDisableEditingBlendMode", HideEditConditionToggle))
	EVoxelVolumeBlendMode BlendMode = EVoxelVolumeBlendMode::Additive;

	UPROPERTY(EditAnywhere, Category = "Config", AdvancedDisplay, meta = (EditCondition = "!bDisableEditingLayers", HideEditConditionToggle))
	TArray<TObjectPtr<UVoxelVolumeLayer>> AdditionalLayers;

public:
	UPROPERTY(VisibleAnywhere, Category = "Config", SkipSerialization, meta = (NoK2))
	bool bDisableEditingLayers = false;

	UPROPERTY(VisibleAnywhere, Category = "Config", SkipSerialization, meta = (NoK2))
	bool bDisableEditingBlendMode = false;

public:
	FVoxelVolumeStamp();

	//~ Begin FVoxelStamp Interface
	virtual void FixupProperties(const IVoxelStampComponentInterface* Interface) override;
#if WITH_EDITOR
	virtual void GetPropertyInfo(FPropertyInfo& Info) const override;
#endif
	//~ End FVoxelStamp Interface
};

USTRUCT()
struct VOXEL_API FVoxelVolumeStampRuntime : public FVoxelStampRuntime
{
	GENERATED_BODY()
	GENERATED_VOXEL_RUNTIME_STAMP_ABSTRACT_BODY(FVoxelVolumeStamp)

public:
	FORCEINLINE EVoxelVolumeBlendMode GetBlendMode() const
	{
		return GetStamp().BlendMode;
	}
	FORCEINLINE float GetSmoothness() const
	{
		return GetStamp().Smoothness;
	}

public:
	virtual void Apply(
		const FVoxelVolumeBulkQuery& Query,
		const FVoxelVolumeTransform& StampToQuery) const;

	virtual void Apply(
		const FVoxelVolumeSparseQuery& Query,
		const FVoxelVolumeTransform& StampToQuery) const
	{
	}
};