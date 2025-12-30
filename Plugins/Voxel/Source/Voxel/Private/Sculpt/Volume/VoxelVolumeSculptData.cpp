// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Volume/VoxelVolumeSculptData.h"
#include "Sculpt/Volume/VoxelVolumeSculptInnerData.h"
#include "VoxelDependency.h"
#include "VoxelTaskContext.h"
#include "VoxelStampRuntime.h"
#include "VoxelInvalidationCallstack.h"

DEFINE_UNIQUE_VOXEL_ID(FVoxelVolumeSculptDataId)

FVoxelVolumeSculptData::FVoxelVolumeSculptData(const bool bUseFastDistances)
	: Dependency(FVoxelDependency3D::Create("FVoxelVolumeSculptData"))
	, bUseFastDistances(bUseFastDistances)
	, InnerData_RequiresLock(MakeShared<FVoxelVolumeSculptInnerData>(bUseFastDistances))
{
}

FVoxelVolumeSculptData::FVoxelVolumeSculptData(const TSharedRef<const FVoxelVolumeSculptInnerData>& InnerData)
	: Dependency(FVoxelDependency3D::Create("FVoxelVolumeSculptData"))
	, bUseFastDistances(InnerData->bUseFastDistances)
	, InnerData_RequiresLock(InnerData)
{
}

TSharedRef<const FVoxelVolumeSculptInnerData> FVoxelVolumeSculptData::GetInnerData() const
{
	VOXEL_SCOPE_READ_LOCK(InnerData_CriticalSection);
	return InnerData_RequiresLock;
}

FVoxelFuture FVoxelVolumeSculptData::AddTask(
	const TFunction<FVoxelBox(FVoxelVolumeSculptInnerData&)> DoWork,
	const TSharedPtr<const FVoxelStampRuntime>& StampRuntime)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	const TSharedRef<FTask> Task = MakeSharedCopy(FTask
	{
		DoWork,
		FVoxelInvalidationCallstack::Create("Apply modifier"),
	});

	{
		FVoxelTaskScope Scope(*GVoxelGlobalTaskContext);

		Future_GameThread = Future_GameThread.Then_AsyncThread(MakeWeakPtrLambda(this, [this, StampRuntime, Task]
		{
			RunTask(StampRuntime, *Task);
		}));
	}

	return Task->Promise;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelVolumeSculptData::RunTask(
	const TSharedPtr<const FVoxelStampRuntime>& StampRuntime,
	const FTask& Task)
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedRef<const FVoxelVolumeSculptInnerData> OldInnerData = GetInnerData();

	const TSharedRef<FVoxelVolumeSculptInnerData> NewInnerData = MakeShared<FVoxelVolumeSculptInnerData>(bUseFastDistances);
	NewInnerData->CopyFrom(*OldInnerData);

	const FVoxelBox BoundsToInvalidate = Task.DoWork(*NewInnerData);

	{
		VOXEL_SCOPE_WRITE_LOCK(InnerData_CriticalSection);

		checkVoxelSlow(InnerData_RequiresLock == OldInnerData);
		InnerData_RequiresLock = NewInnerData;
	}

	Voxel::GameTask([Dependency = Dependency, Callstack = Task.Callstack, StampRuntime, BoundsToInvalidate]
	{
		VOXEL_FUNCTION_COUNTER();

		const FVoxelInvalidationScope Scope(Callstack);

		Dependency->Invalidate(BoundsToInvalidate);

		if (!StampRuntime)
		{
			return;
		}

		// Request update right after invalidating on the game thread, otherwise an update could go through in-between
		// with the old InnerData
		StampRuntime->RequestUpdate();

		if (!GIsEditor)
		{
			return;
		}

		if (const USceneComponent* Component = StampRuntime->GetComponent().Resolve_Ensured())
		{
			(void)Component->MarkPackageDirty();
		}
	});

	// Make sure to do this AFTER setting InnerData_RequiresLock
	Task.Promise.Set();
}