// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelVolumeStampRef.h"
#include "VoxelVolumeSplineStamp.h"
#include "VoxelVolumeSplineStampRef.generated.h"

////////////////////////////////////////////////////
///////// The code below is auto-generated /////////
////////////////////////////////////////////////////

USTRUCT(BlueprintType, DisplayName = "Voxel Volume Spline Stamp", Category = "Voxel|Stamp|Volume Spline", meta = (HasNativeMake = "/Script/Voxel.VoxelVolumeSplineStamp_K2.Make", HasNativeBreak = "/Script/Voxel.VoxelVolumeSplineStamp_K2.Break"))
struct VOXEL_API FVoxelVolumeSplineStampRef final : public FVoxelVolumeStampRef
{
	GENERATED_BODY()
	GENERATED_VOXEL_STAMP_REF_BODY(FVoxelVolumeSplineStamp)
};

template<>
struct TStructOpsTypeTraits<FVoxelVolumeSplineStampRef> : TStructOpsTypeTraits<FVoxelStampRef>
{
};

template<>
struct TVoxelStampRefImpl<FVoxelVolumeSplineStamp>
{
	using Type = FVoxelVolumeSplineStampRef;
};