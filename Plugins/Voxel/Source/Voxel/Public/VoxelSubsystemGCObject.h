// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

struct VOXEL_API IVoxelSubsystemGCObject
{
	virtual ~IVoxelSubsystemGCObject() = default;

	virtual void AddReferencedObjects(FReferenceCollector& Collector) = 0;
};