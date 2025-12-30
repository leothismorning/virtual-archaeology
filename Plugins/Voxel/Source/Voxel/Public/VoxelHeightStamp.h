// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLayer.h"
#include "VoxelStamp.h"
#include "VoxelStampQuery.h"
#include "VoxelStampRuntime.h"
#include "VoxelStampTransform.h"
#include "VoxelHeightBlendMode.h"
#include "VoxelHeightStamp.generated.h"

class FVoxelQuery;
class FVoxelMetadataView;

USTRUCT(meta = (Abstract))
struct VOXEL_API FVoxelHeightStamp : public FVoxelStamp
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	// Layer that this stamps belong to
	// You can control the order of layers in Layer Stacks
	// You can select the layer stack to use in your Voxel World or PCG Sampler settings
	UPROPERTY(EditAnywhere, Category = "Config", meta = (EditCondition = "!bDisableEditingLayers", HideEditConditionToggle))
	TObjectPtr<UVoxelHeightLayer> Layer;

	UPROPERTY()
	TArray<TObjectPtr<UVoxelHeightLayer>> VoxelLayers;

	UPROPERTY(EditAnywhere, Category = "Config", meta = (EditCondition = "!bDisableEditingBlendMode", HideEditConditionToggle))
	EVoxelHeightBlendMode BlendMode = EVoxelHeightBlendMode::Max;

	UPROPERTY(EditAnywhere, Category = "Config", AdvancedDisplay, meta = (EditCondition = "!bDisableEditingLayers", HideEditConditionToggle))
	TArray<TObjectPtr<UVoxelHeightLayer>> AdditionalLayers;

public:
	UPROPERTY(VisibleAnywhere, Category = "Config", SkipSerialization, meta = (NoK2))
	bool bDisableEditingLayers = false;

	UPROPERTY(VisibleAnywhere, Category = "Config", SkipSerialization, meta = (NoK2))
	bool bDisableEditingBlendMode = false;

public:
	FVoxelHeightStamp();

	//~ Begin FVoxelStamp Interface
	virtual void FixupProperties(const IVoxelStampComponentInterface* Interface) override;
#if WITH_EDITOR
	virtual void GetPropertyInfo(FPropertyInfo& Info) const override;
#endif
	//~ End FVoxelStamp Interface
};

USTRUCT()
struct VOXEL_API FVoxelHeightStampRuntime : public FVoxelStampRuntime
{
	GENERATED_BODY()
	GENERATED_VOXEL_RUNTIME_STAMP_ABSTRACT_BODY(FVoxelHeightStamp)

public:
	FORCEINLINE EVoxelHeightBlendMode GetBlendMode() const
	{
		return GetStamp().BlendMode;
	}
	FORCEINLINE float GetSmoothness() const
	{
		return GetStamp().Smoothness;
	}

public:
	virtual bool HasRelativeHeightRange() const
	{
		return false;
	}

public:
	virtual void Apply(
		const FVoxelHeightBulkQuery& Query,
		const FVoxelHeightTransform& StampToQuery) const;

	virtual void Apply(
		const FVoxelHeightSparseQuery& Query,
		const FVoxelHeightTransform& StampToQuery) const
	{
	}
};