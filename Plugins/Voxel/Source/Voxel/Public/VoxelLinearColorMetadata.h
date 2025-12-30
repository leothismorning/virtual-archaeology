// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMetadata.h"
#include "Buffer/VoxelFloatBuffers.h"
#include "VoxelLinearColorMetadataRef.h"
#include "VoxelLinearColorMetadata.generated.h"

UCLASS()
class VOXEL_API UVoxelLinearColorMetadata : public UVoxelMetadata
{
	GENERATED_BODY()
	GENERATED_VOXEL_METADATA_BODY(FVoxelLinearColorMetadataRef)

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	FLinearColor DefaultValue;

public:
	//~ Begin UVoxelMetadata Interface
	virtual FVoxelPinValue GetDefaultValue() const override;
	virtual TVoxelOptional<EVoxelMetadataMaterialType> GetMaterialType() const override;
	//~ End UVoxelMetadata Interface
};