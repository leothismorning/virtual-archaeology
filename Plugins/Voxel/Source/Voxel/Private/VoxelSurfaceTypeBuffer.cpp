// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Surface/VoxelSurfaceTypeBuffer.h"

void FVoxelSurfaceTypePinType::Convert(
	const bool bSetObject,
	TVoxelObjectPtr<UVoxelSurfaceTypeInterface>& OutObject,
	UVoxelSurfaceTypeInterface& InObject,
	FVoxelSurfaceType& Struct)
{
	if (bSetObject)
	{
		if (Struct.IsNull())
		{
			OutObject = nullptr;
		}
		else
		{
			OutObject = Struct.GetSurfaceTypeInterface();
		}
	}
	else
	{
		Struct = FVoxelSurfaceType(&InObject);
	}
}