// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelHeightStampRef.h"
#include "VoxelHeightSculptStamp.h"
#include "VoxelHeightSculptStampRef.generated.h"

////////////////////////////////////////////////////
///////// The code below is auto-generated /////////
////////////////////////////////////////////////////

USTRUCT(BlueprintType, DisplayName = "Voxel Height Sculpt Stamp", Category = "Voxel|Stamp|Height Sculpt", meta = (HasNativeMake = "/Script/Voxel.VoxelHeightSculptStamp_K2.Make", HasNativeBreak = "/Script/Voxel.VoxelHeightSculptStamp_K2.Break"))
struct VOXEL_API FVoxelHeightSculptStampRef final : public FVoxelHeightStampRef
{
	GENERATED_BODY()
	GENERATED_VOXEL_STAMP_REF_BODY(FVoxelHeightSculptStamp)
};

template<>
struct TStructOpsTypeTraits<FVoxelHeightSculptStampRef> : TStructOpsTypeTraits<FVoxelStampRef>
{
};

template<>
struct TVoxelStampRefImpl<FVoxelHeightSculptStamp>
{
	using Type = FVoxelHeightSculptStampRef;
};