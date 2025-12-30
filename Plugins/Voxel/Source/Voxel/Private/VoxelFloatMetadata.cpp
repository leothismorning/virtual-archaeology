// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelFloatMetadata.h"

DEFINE_VOXEL_FACTORY(UVoxelFloatMetadata);

FVoxelPinValue UVoxelFloatMetadata::GetDefaultValue() const
{
	return FVoxelPinValue::Make(DefaultValue);
}

TVoxelOptional<EVoxelMetadataMaterialType> UVoxelFloatMetadata::GetMaterialType() const
{
	return EVoxelMetadataMaterialType::Float1;
}