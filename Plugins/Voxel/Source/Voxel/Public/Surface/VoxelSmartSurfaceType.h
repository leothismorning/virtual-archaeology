// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNodeEvaluator.h"
#include "VoxelSmartSurfacePreviewShape.h"
#include "VoxelParameterOverridesOwner.h"
#include "Surface/VoxelSurfaceTypeInterface.h"
#include "VoxelSmartSurfaceType.generated.h"

class UVoxelSurfaceTypeGraph;
class FVoxelSmartSurfaceProxy;
struct FVoxelOutputNode_OutputSurface;

UCLASS(BlueprintType, meta = (VoxelAssetType, AssetColor=Green))
class VOXEL_API UVoxelSmartSurfaceType
	: public UVoxelSurfaceTypeInterface
	, public IVoxelParameterOverridesObjectOwner
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	TObjectPtr<UVoxelSurfaceTypeGraph> Graph;

	UPROPERTY()
	FVoxelParameterOverrides ParameterOverrides;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	EVoxelSmartSurfacePreviewShape PreviewShape = EVoxelSmartSurfacePreviewShape::Sphere;
#endif

public:
	//~ Begin IVoxelParameterOverridesObjectOwner Interface
	virtual bool ShouldForceEnableOverride(const FGuid& Guid) const override;
	virtual UVoxelGraph* GetGraph() const override;
	virtual FVoxelParameterOverrides& GetParameterOverrides() override;
	//~ End IVoxelParameterOverridesObjectOwner Interface

	//~ Begin UObject Interface
	virtual void PostLoad() override;
	virtual void PostInitProperties() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface

public:
	FVoxelDependency& GetDependency();
	TVoxelNodeEvaluator<FVoxelOutputNode_OutputSurface> CreateEvaluator(FVoxelDependencyCollector& DependencyCollector) const;

private:
	TSharedPtr<FVoxelDependency> Dependency;
	FSharedVoidPtr OnChangedPtr;

	UPROPERTY(Transient, NonTransactional)
	FVoxelParameterOverrides LastParameterOverrides;
};