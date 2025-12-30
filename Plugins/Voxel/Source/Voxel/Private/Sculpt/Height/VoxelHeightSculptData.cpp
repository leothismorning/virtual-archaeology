// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Height/VoxelHeightSculptData.h"
#include "Sculpt/Height/VoxelHeightSculptInnerData.h"
#include "VoxelDependency.h"
#include "VoxelTaskContext.h"
#include "VoxelStampRuntime.h"
#include "VoxelInvalidationCallstack.h"

DEFINE_UNIQUE_VOXEL_ID(FVoxelHeightSculptDataId)

FVoxelHeightSculptData::FVoxelHeightSculptData()
	: Dependency(FVoxelDependency2D::Create("FVoxelHeightSculptData"))
	, InnerData_RequiresLock(MakeShared<FVoxelHeightSculptInnerData>())
{
}

FVoxelHeightSculptData::FVoxelHeightSculptData(const TSharedRef<const FVoxelHeightSculptInnerData>& InnerData)
	: Dependency(FVoxelDependency2D::Create("FVoxelHeightSculptData"))
	, InnerData_RequiresLock(InnerData)
{
}

TSharedRef<const FVoxelHeightSculptInnerData> FVoxelHeightSculptData::GetInnerData() const
{
	VOXEL_SCOPE_READ_LOCK(InnerData_CriticalSection);
	return InnerData_RequiresLock;
}

FVoxelFuture FVoxelHeightSculptData::AddTask(
	const TFunction<FVoxelBox2D(FVoxelHeightSculptInnerData&)> DoWork,
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

void FVoxelHeightSculptData::RunTask(
	const TSharedPtr<const FVoxelStampRuntime>& StampRuntime,
	const FTask& Task)
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedRef<const FVoxelHeightSculptInnerData> OldInnerData = GetInnerData();

	const TSharedRef<FVoxelHeightSculptInnerData> NewInnerData = MakeShared<FVoxelHeightSculptInnerData>();
	NewInnerData->CopyFrom(*OldInnerData);

	const FVoxelBox2D BoundsToInvalidate = Task.DoWork(*NewInnerData);

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