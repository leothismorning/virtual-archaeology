// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelVolumeStampRef.h"
#include "VoxelVolumeGraphStamp.h"
#include "VoxelVolumeGraphStampRef.generated.h"

////////////////////////////////////////////////////
///////// The code below is auto-generated /////////
////////////////////////////////////////////////////

USTRUCT(BlueprintType, DisplayName = "Voxel Volume Graph Stamp", Category = "Voxel|Stamp|Volume Graph", meta = (HasNativeMake = "/Script/Voxel.VoxelVolumeGraphStamp_K2.Make", HasNativeBreak = "/Script/Voxel.VoxelVolumeGraphStamp_K2.Break"))
struct VOXEL_API FVoxelVolumeGraphStampRef final : public FVoxelVolumeStampRef
{
	GENERATED_BODY()
	GENERATED_VOXEL_STAMP_REF_BODY(FVoxelVolumeGraphStamp)
};

template<>
struct TStructOpsTypeTraits<FVoxelVolumeGraphStampRef> : TStructOpsTypeTraits<FVoxelStampRef>
{
};

template<>
struct TVoxelStampRefImpl<FVoxelVolumeGraphStamp>
{
	using Type = FVoxelVolumeGraphStampRef;
};