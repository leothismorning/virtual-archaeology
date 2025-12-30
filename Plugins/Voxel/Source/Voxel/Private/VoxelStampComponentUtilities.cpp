// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelStampComponentUtilities.h"
#include "VoxelHeightStamp.h"
#include "VoxelVolumeStamp.h"

bool FVoxelStampComponentUtilities::ShouldRender(const USceneComponent* Component)
{
	if (!IsValid(Component) ||
		Component->IsTemplate() ||
		!Component->IsVisible() ||
		!Component->IsRegistered())
	{
		return false;
	}

	const AActor* Owner = Component->GetOwner();
	if (!Owner)
	{
		return false;
	}

	if (Owner->HasAnyFlags(RF_ClassDefaultObject))
	{
		// Likely in a blueprint CDO
		return false;
	}

#if WITH_EDITOR
	if (Owner->IsHiddenEd())
	{
		return false;
	}
#endif

	return true;
}

FVoxelBox FVoxelStampComponentUtilities::GetLocalBounds(const FVoxelStampRuntime& Stamp)
{
	VOXEL_FUNCTION_COUNTER();

	if (Stamp.FailedToInitialize())
	{
		return {};
	}

	return Stamp.GetLocalBounds();
}