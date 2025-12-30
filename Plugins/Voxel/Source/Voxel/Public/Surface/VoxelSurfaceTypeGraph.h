// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelGraph.h"
#include "VoxelSmartSurfacePreviewShape.h"
#include "VoxelSurfaceTypeGraph.generated.h"

class UVoxelSmartSurfaceType;

UCLASS(BlueprintType, meta = (AssetSubMenu = "Graph"))
class VOXEL_API UVoxelSurfaceTypeGraph : public UVoxelGraph
{
	GENERATED_BODY()

public:
	//~ Begin UVoxelGraph Interface
#if WITH_EDITOR
	virtual FFactoryInfo GetFactoryInfo() override;
#endif
	virtual UScriptStruct* GetOutputNodeStruct() const override;
	//~ End UVoxelGraph Interface

	UVoxelSmartSurfaceType* GetPreviewSurface();

public:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	EVoxelSmartSurfacePreviewShape PreviewShape = EVoxelSmartSurfacePreviewShape::Sphere;
#endif

private:
	UPROPERTY(Transient)
	TObjectPtr<UVoxelSmartSurfaceType> PreviewSurfaceType;
};