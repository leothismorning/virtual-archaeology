// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelPCGFunctionLibrary.h"

FVoxelBox UVoxelPCGFunctionLibrary::GetPCGBounds() const
{
	const FVoxelGraphParameters::FPCGBounds* Parameter = Query->FindParameter<FVoxelGraphParameters::FPCGBounds>();
	if (!Parameter)
	{
		VOXEL_MESSAGE(Error, "{0}: No bounds passed in input", this);
		return {};
	}

	if (!Parameter->Bounds.IsValidAndNotEmpty())
	{
		VOXEL_MESSAGE(Error, "{0}: No or invalid bounds passed in input", this);
		return {};
	}

	return Parameter->Bounds;
}