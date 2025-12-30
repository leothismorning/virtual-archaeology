// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelHeightStamp.h"
#include "VoxelHeightSculptStamp.generated.h"

class UVoxelLayerStack;
class FVoxelHeightSculptData;
class FVoxelHeightSculptCache;
class FVoxelHeightSculptInnerData;
struct FVoxelHeightModifier;

DECLARE_UNIQUE_VOXEL_ID(FVoxelHeightSculptDataId);

USTRUCT(meta = (Internal))
struct VOXEL_API FVoxelHeightSculptStamp final : public FVoxelHeightStamp
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	float ScaleXY = 100;

	// If true height will be stored relative to the previous stamp heights
	UPROPERTY(EditAnywhere, Category = "Config", AdvancedDisplay)
	bool bRelativeHeight = false;

	// Use this if this stamp is not rendered in the Voxel World stack
	// This stack will be used during sculpting to query the distances before any sculpt is applied
	UPROPERTY(EditAnywhere, Category = "Config", AdvancedDisplay)
	TObjectPtr<UVoxelLayerStack> StackOverride;

public:
	FVoxelHeightSculptStamp();

	FVoxelFuture ApplyModifier(const TSharedRef<const FVoxelHeightModifier>& Modifier);
	FVoxelFuture SetInnerData(const TSharedRef<const FVoxelHeightSculptInnerData>& NewInnerData);
	FVoxelFuture ClearSculptData();

	void ClearCache();

	TSharedRef<FVoxelHeightSculptData> GetData() const;
	void SetData(const TSharedRef<FVoxelHeightSculptData>& NewData);

public:
	//~ Begin FVoxelHeightStamp Interface
	virtual void PostDuplicate() override;
#if WITH_EDITOR
	virtual void GetActorLabelInfo(FActorLabelInfo& Info) const override;
	virtual void GetPropertyInfo(FPropertyInfo& Info) const override;
#endif
	//~ End FVoxelHeightStamp Interface

private:
	TSharedRef<FVoxelHeightSculptData> Data;
	TSharedPtr<FVoxelHeightSculptCache> Cache;
};

USTRUCT()
struct VOXEL_API FVoxelHeightSculptStampRuntime : public FVoxelHeightStampRuntime
{
	GENERATED_BODY()
	GENERATED_VOXEL_RUNTIME_STAMP_BODY(FVoxelHeightSculptStamp)

	FVoxelHeightSculptDataId SculptDataId;
	TSharedPtr<FVoxelDependency2D> Dependency;
	TSharedPtr<const FVoxelHeightSculptInnerData> InnerData;

	//~ Begin FVoxelHeightStampRuntime Interface
	virtual bool Initialize(FVoxelDependencyCollector& DependencyCollector) override;
	virtual FVoxelBox GetLocalBounds() const override;
	virtual bool HasRelativeHeightRange() const override;

	virtual bool ShouldFullyInvalidate(
		const FVoxelStampRuntime& PreviousRuntime,
		TVoxelArray<FVoxelBox>& OutLocalBoundsToInvalidate) const override;

	virtual void Apply(
		const FVoxelHeightBulkQuery& Query,
		const FVoxelHeightTransform& StampToQuery) const override;

	virtual void Apply(
		const FVoxelHeightSparseQuery& Query,
		const FVoxelHeightTransform& StampToQuery) const override;
	//~ End FVoxelHeightStampRuntime Interface
};