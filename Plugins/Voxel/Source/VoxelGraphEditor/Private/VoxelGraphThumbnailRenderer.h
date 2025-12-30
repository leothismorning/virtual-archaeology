// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelThumbnailRenderers.h"
#include "VoxelGraphThumbnailRenderer.generated.h"

UCLASS()
class VOXELGRAPHEDITOR_API UVoxelGraphThumbnailRenderer : public UVoxelTextureWithBackgroundRenderer
{
	GENERATED_BODY()

	//~ Begin UVoxelTextureThumbnailRenderer Interface
	virtual FVoxelAssetIcon GetAssetIcon(UObject* Object) const override;
	//~ End UVoxelTextureThumbnailRenderer Interface
};