// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelLinearColorMetadata.h"

DEFINE_VOXEL_FACTORY(UVoxelLinearColorMetadata);

FVoxelPinValue UVoxelLinearColorMetadata::GetDefaultValue() const
{
	return FVoxelPinValue::Make(DefaultValue);
}

TVoxelOptional<EVoxelMetadataMaterialType> UVoxelLinearColorMetadata::GetMaterialType() const
{
	return EVoxelMetadataMaterialType::Float4;
}