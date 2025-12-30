// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelStamp.h"
#include "VoxelStampRef.h"
#include "VoxelStampRuntime.h"

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelStamp);

void FVoxelStamp::FixupProperties(const IVoxelStampComponentInterface* Interface)
{
	MetadataOverrides.Fixup();
}

#if WITH_EDITOR
FVoxelStamp::FActorLabelInfo FVoxelStamp::GetActorLabelInfo() const
{
	FActorLabelInfo Info;
	GetActorLabelInfo(Info);
	return Info;
}
#endif

FVoxelStampRef FVoxelStamp::GetStampRef() const
{
	const FVoxelStampRef StampRef = WeakStampRef.Pin();
	if (!StampRef ||
		// Stamp was changed in-between
		StampRef.GetStampData() != this)
	{
		return {};
	}

	return StampRef;
}

TVoxelObjectPtr<USceneComponent> FVoxelStamp::GetComponent() const
{
	const TSharedPtr<const FVoxelStampRuntime> Runtime = ResolveStampRuntime();
	if (!Runtime)
	{
		return {};
	}

	return Runtime->GetComponent();
}

TSharedPtr<const FVoxelStampRuntime> FVoxelStamp::ResolveStampRuntime() const
{
	const FVoxelStampRef StampRef = GetStampRef();
	if (!StampRef)
	{
		return {};
	}

	const TSharedPtr<const FVoxelStampRuntime> Runtime = StampRef.ResolveStampRuntime();
	ensure(Runtime || !StampRef.IsRegistered());
	return Runtime;
}