// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelThumbnailRenderers.h"
#include "VoxelTextureThumbnailRenderer.generated.h"

UCLASS()
class VOXELEDITOR_API UVoxelTextureAssetThumbnailRenderer : public UVoxelTextureThumbnailRenderer
{
	GENERATED_BODY()

	//~ Begin UVoxelTextureThumbnailRenderer Interface
	virtual bool CanVisualizeAsset(UObject* Object) override;
	virtual UTexture* GetTexture(UObject* Object) const override;
	//~ End UVoxelTextureThumbnailRenderer Interface
};