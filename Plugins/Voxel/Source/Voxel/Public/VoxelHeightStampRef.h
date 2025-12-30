// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelStampRef.h"
#include "VoxelHeightStamp.h"
#include "VoxelHeightStampRef.generated.h"

////////////////////////////////////////////////////
///////// The code below is auto-generated /////////
////////////////////////////////////////////////////

USTRUCT(BlueprintType, DisplayName = "Voxel Height Stamp", Category = "Voxel|Stamp|Height", meta = (HasNativeMake = "/Script/Voxel.VoxelHeightStamp_K2.Make", HasNativeBreak = "/Script/Voxel.VoxelHeightStamp_K2.Break"))
struct VOXEL_API FVoxelHeightStampRef : public FVoxelStampRef
{
	GENERATED_BODY()
	GENERATED_VOXEL_STAMP_REF_PARENT_BODY(FVoxelHeightStamp)
};

template<>
struct TStructOpsTypeTraits<FVoxelHeightStampRef> : TStructOpsTypeTraits<FVoxelStampRef>
{
};

template<>
struct TVoxelStampRefImpl<FVoxelHeightStamp>
{
	using Type = FVoxelHeightStampRef;
};