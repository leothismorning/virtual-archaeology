// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

struct FVoxelStampRef;
struct FVoxelStampRuntime;
class FVoxelVolumeSculptEditor;
class FVoxelVolumeSculptInnerData;

DECLARE_UNIQUE_VOXEL_ID(FVoxelVolumeSculptDataId);

class VOXEL_API FVoxelVolumeSculptData : public TSharedFromThis<FVoxelVolumeSculptData>
{
public:
	const FVoxelVolumeSculptDataId SculptDataId = FVoxelVolumeSculptDataId::New();
	const TSharedRef<FVoxelDependency3D> Dependency;
	const bool bUseFastDistances;

	explicit FVoxelVolumeSculptData(bool bUseFastDistances);
	explicit FVoxelVolumeSculptData(const TSharedRef<const FVoxelVolumeSculptInnerData>& InnerData);

	TSharedRef<const FVoxelVolumeSculptInnerData> GetInnerData() const;

	FVoxelFuture AddTask(
		TFunction<FVoxelBox(FVoxelVolumeSculptInnerData&)> DoWork,
		const TSharedPtr<const FVoxelStampRuntime>& StampRuntime);

private:
	FVoxelSharedCriticalSection InnerData_CriticalSection;
	TSharedRef<const FVoxelVolumeSculptInnerData> InnerData_RequiresLock;

private:
	struct FTask
	{
		TFunction<FVoxelBox(FVoxelVolumeSculptInnerData&)> DoWork;
		TSharedRef<const FVoxelInvalidationCallstack> Callstack;
		FVoxelPromise Promise;
	};
	FVoxelFuture Future_GameThread;

	void RunTask(
		const TSharedPtr<const FVoxelStampRuntime>& StampRuntime,
		const FTask& Task);
};