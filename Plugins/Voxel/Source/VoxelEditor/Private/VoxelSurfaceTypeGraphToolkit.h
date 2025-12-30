// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoxelSimpleAssetToolkit.h"
#include "VoxelStampGraphViewportInterface.h"
#include "VoxelSurfaceTypeGraphToolkit.generated.h"

class UVoxelSurfaceTypeGraph;

class FVoxelSurfaceTypeGraphViewportInterface : public FVoxelStampGraphViewportInterface
{
public:
	//~ Begin FVoxelStampGraphViewportInterface Interface
	virtual void PopulateOverlay(const TSharedRef<SOverlay>& Overlay) override;
	virtual void SetupVoxelWorld(AVoxelWorld& VoxelWorld) const override;
	virtual FVoxelStampRef MakeStampRef(UVoxelGraph& Graph) const override;
	virtual TOptional<float> GetBoundsSize(const FVoxelStampRef& StampRef) const override;
#if VOXEL_ENGINE_VERSION >= 506
	virtual void ExtendToolbar(UToolMenu& ToolMenu) override;
#endif
	//~ End FVoxelStampGraphViewportInterface Interface
};

USTRUCT()
struct FVoxelSurfaceTypeGraphToolkit : public FVoxelToolkit
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	UPROPERTY()
	TObjectPtr<UVoxelSurfaceTypeGraph> Asset;

public:
	//~ Begin FVoxelToolkit Interface
	virtual TArray<FMode> GetModes() const override;
	virtual UScriptStruct* GetDefaultMode() const override;
	//~ End FVoxelToolkit Interface
};

USTRUCT(meta = (Internal))
struct FVoxelSurfaceTypeGraphPreviewToolkit : public FVoxelSimpleAssetToolkit
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	UPROPERTY()
	TObjectPtr<UVoxelSurfaceTypeGraph> Asset;

public:
	//~ Begin FVoxelSimpleAssetToolkit Interface
	virtual bool ShowFloor() const override { return false; }
	virtual void SetupPreview() override;
	virtual FRotator GetInitialViewRotation() const override;
	virtual TOptional<float> GetInitialViewDistance() const override;
	virtual void PostEditChange(const FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual TSharedPtr<SWidget> GetMenuOverlay() const override;
	//~ End FVoxelSimpleAssetToolkit Interface

private:
	TSharedPtr<FVoxelSurfaceTypeGraphViewportInterface> ViewportInterface;
};