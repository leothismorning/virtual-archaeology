// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelStampRef.h"
#include "VoxelVolumeStamp.h"
#include "VoxelVolumeStampRef.generated.h"

////////////////////////////////////////////////////
///////// The code below is auto-generated /////////
////////////////////////////////////////////////////

USTRUCT(BlueprintType, DisplayName = "Voxel Volume Stamp", Category = "Voxel|Stamp|Volume", meta = (HasNativeMake = "/Script/Voxel.VoxelVolumeStamp_K2.Make", HasNativeBreak = "/Script/Voxel.VoxelVolumeStamp_K2.Break"))
struct VOXEL_API FVoxelVolumeStampRef : public FVoxelStampRef
{
	GENERATED_BODY()
	GENERATED_VOXEL_STAMP_REF_PARENT_BODY(FVoxelVolumeStamp)
};

template<>
struct TStructOpsTypeTraits<FVoxelVolumeStampRef> : TStructOpsTypeTraits<FVoxelStampRef>
{
};

template<>
struct TVoxelStampRefImpl<FVoxelVolumeStamp>
{
	using Type = FVoxelVolumeStampRef;
};