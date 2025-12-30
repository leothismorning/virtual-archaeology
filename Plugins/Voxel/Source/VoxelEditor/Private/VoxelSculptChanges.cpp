// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelSculptChanges.h"
#include "VoxelStampComponent.h"
#include "Sculpt/Height/VoxelHeightSculptData.h"
#include "Sculpt/Height/VoxelHeightSculptActor.h"
#include "Sculpt/Height/VoxelHeightSculptInnerData.h"
#include "Sculpt/Volume/VoxelVolumeSculptData.h"
#include "Sculpt/Volume/VoxelVolumeSculptActor.h"
#include "Sculpt/Volume/VoxelVolumeSculptInnerData.h"

FString FVoxelHeightSculptChange::ToString() const
{
	return "Sculpt height";
}

TUniquePtr<FChange> FVoxelHeightSculptChange::Execute(UObject* Object)
{
	VOXEL_FUNCTION_COUNTER();

	const UVoxelHeightSculptComponent* Component = Cast<UVoxelHeightSculptComponent>(Object);
	if (!ensure(Component))
	{
		return MakeUnique<FVoxelHeightSculptChange>(MakeShared<FVoxelHeightSculptInnerData>());
	}

	TUniquePtr<FVoxelHeightSculptChange> Result = MakeUnique<FVoxelHeightSculptChange>(Component->GetStamp()->GetData()->GetInnerData());
	Component->GetStamp()->SetInnerData(Snapshot);
	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString FVoxelVolumeSculptChange::ToString() const
{
	return "Sculpt volume";
}

TUniquePtr<FChange> FVoxelVolumeSculptChange::Execute(UObject* Object)
{
	VOXEL_FUNCTION_COUNTER();

	const UVoxelVolumeSculptComponent* Component = Cast<UVoxelVolumeSculptComponent>(Object);
	if (!ensure(Component))
	{
		return MakeUnique<FVoxelVolumeSculptChange>(MakeShared<FVoxelVolumeSculptInnerData>(false));
	}

	TUniquePtr<FVoxelVolumeSculptChange> Result = MakeUnique<FVoxelVolumeSculptChange>(Component->GetStamp()->GetData()->GetInnerData());
	Component->GetStamp()->SetInnerData(Snapshot);
	return Result;
}