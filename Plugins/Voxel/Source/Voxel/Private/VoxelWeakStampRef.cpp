// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelWeakStampRef.h"
#include "VoxelStampRef.h"

FVoxelWeakStampRef::FVoxelWeakStampRef(const FVoxelStampRef& StampRef)
	: WeakInner(StampRef.Inner.SharedRef)
{
}

FVoxelStampRef FVoxelWeakStampRef::Pin() const
{
	const TSharedPtr<FVoxelStampRefInner> Inner = WeakInner.Pin();
	if (!Inner)
	{
		return {};
	}

	return FVoxelStampRef(Inner.ToSharedRef());
}