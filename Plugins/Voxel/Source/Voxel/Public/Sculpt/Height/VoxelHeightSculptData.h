// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

struct FVoxelStampRef;
struct FVoxelStampRuntime;
class FVoxelHeightSculptEditor;
class FVoxelHeightSculptInnerData;

DECLARE_UNIQUE_VOXEL_ID(FVoxelHeightSculptDataId);

class VOXEL_API FVoxelHeightSculptData : public TSharedFromThis<FVoxelHeightSculptData>
{
public:
	const FVoxelHeightSculptDataId SculptDataId = FVoxelHeightSculptDataId::New();
	const TSharedRef<FVoxelDependency2D> Dependency;

	FVoxelHeightSculptData();
	explicit FVoxelHeightSculptData(const TSharedRef<const FVoxelHeightSculptInnerData>& InnerData);

	TSharedRef<const FVoxelHeightSculptInnerData> GetInnerData() const;

	FVoxelFuture AddTask(
		TFunction<FVoxelBox2D(FVoxelHeightSculptInnerData&)> DoWork,
		const TSharedPtr<const FVoxelStampRuntime>& StampRuntime);

private:
	FVoxelSharedCriticalSection InnerData_CriticalSection;
	TSharedRef<const FVoxelHeightSculptInnerData> InnerData_RequiresLock;

private:
	struct FTask
	{
		TFunction<FVoxelBox2D(FVoxelHeightSculptInnerData&)> DoWork;
		TSharedRef<const FVoxelInvalidationCallstack> Callstack;
		FVoxelPromise Promise;
	};
	FVoxelFuture Future_GameThread;

	void RunTask(
		const TSharedPtr<const FVoxelStampRuntime>& StampRuntime,
		const FTask& Task);
};