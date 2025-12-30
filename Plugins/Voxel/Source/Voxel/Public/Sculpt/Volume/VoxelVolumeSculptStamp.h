// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelVolumeStamp.h"
#include "VoxelVolumeSculptStamp.generated.h"

class UVoxelLayerStack;
class FVoxelVolumeSculptData;
class FVoxelVolumeSculptCache;
class FVoxelVolumeSculptInnerData;
struct FVoxelVolumeModifier;

DECLARE_UNIQUE_VOXEL_ID(FVoxelVolumeSculptDataId);

USTRUCT(meta = (Internal))
struct VOXEL_API FVoxelVolumeSculptStamp final : public FVoxelVolumeStamp
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	float Scale = 100;

	// If true will compress distances to one byte
	// Setting this will clear any existing data
	UPROPERTY(EditAnywhere, Category = "Config", AdvancedDisplay)
	bool bUseFastDistances = false;

	// If false, edits won't be diffed
	// This make editing up to 5x faster, but will lead to obvious chunks if you move the underlying stamps after editing
	UPROPERTY(EditAnywhere, Category = "Config", AdvancedDisplay)
	bool bEnableDiffing = true;

	// Use this if this stamp is not rendered in the Voxel World stack
	// This stack will be used during sculpting to query the distances before any sculpt is applied
	UPROPERTY(EditAnywhere, Category = "Config", AdvancedDisplay)
	TObjectPtr<UVoxelLayerStack> StackOverride;

public:
	FVoxelVolumeSculptStamp();

	FVoxelFuture ApplyModifier(const TSharedRef<const FVoxelVolumeModifier>& Modifier);
	FVoxelFuture SetInnerData(const TSharedRef<const FVoxelVolumeSculptInnerData>& NewInnerData);
	FVoxelFuture ClearSculptData();

	void ClearCache();

	TSharedRef<FVoxelVolumeSculptData> GetData() const;
	void SetData(const TSharedRef<FVoxelVolumeSculptData>& NewData);

public:
	//~ Begin FVoxelVolumeStamp Interface
	virtual void FixupProperties(const IVoxelStampComponentInterface* Interface) override;
	virtual void PostDuplicate() override;
#if WITH_EDITOR
	virtual void GetActorLabelInfo(FActorLabelInfo& Info) const override;
	virtual void GetPropertyInfo(FPropertyInfo& Info) const override;
#endif
	//~ End FVoxelVolumeStamp Interface

private:
	mutable TSharedPtr<FVoxelVolumeSculptData> PrivateData;
	TSharedPtr<FVoxelVolumeSculptCache> Cache;
};

USTRUCT()
struct VOXEL_API FVoxelVolumeSculptStampRuntime : public FVoxelVolumeStampRuntime
{
	GENERATED_BODY()
	GENERATED_VOXEL_RUNTIME_STAMP_BODY(FVoxelVolumeSculptStamp)

	FVoxelVolumeSculptDataId SculptDataId;
	TSharedPtr<FVoxelDependency3D> Dependency;
	TSharedPtr<const FVoxelVolumeSculptInnerData> InnerData;

	//~ Begin FVoxelVolumeStampRuntime Interface
	virtual bool Initialize(FVoxelDependencyCollector& DependencyCollector) override;
	virtual FVoxelBox GetLocalBounds() const override;

	virtual bool ShouldFullyInvalidate(
		const FVoxelStampRuntime& PreviousRuntime,
		TVoxelArray<FVoxelBox>& OutLocalBoundsToInvalidate) const override;

	virtual void Apply(
		const FVoxelVolumeBulkQuery& Query,
		const FVoxelVolumeTransform& StampToQuery) const override;

	virtual void Apply(
		const FVoxelVolumeSparseQuery& Query,
		const FVoxelVolumeTransform& StampToQuery) const override;
	//~ End FVoxelVolumeStampRuntime Interface
};