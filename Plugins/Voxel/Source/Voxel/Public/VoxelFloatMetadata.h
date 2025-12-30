// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMetadata.h"
#include "VoxelFloatMetadataRef.h"
#include "Buffer/VoxelBaseBuffers.h"
#include "VoxelFloatMetadata.generated.h"

UCLASS()
class VOXEL_API UVoxelFloatMetadata : public UVoxelMetadata
{
	GENERATED_BODY()
	GENERATED_VOXEL_METADATA_BODY(FVoxelFloatMetadataRef)

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	float DefaultValue = 0.f;

public:
	//~ Begin UVoxelMetadata Interface
	virtual FVoxelPinValue GetDefaultValue() const override;
	virtual TVoxelOptional<EVoxelMetadataMaterialType> GetMaterialType() const override;
	//~ End UVoxelMetadata Interface

private:
	UPROPERTY()
	FName LegacyName;

	friend UVoxelMetadata;
};