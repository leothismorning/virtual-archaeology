// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelHeightStampRef.h"
#include "VoxelHeightGraphStamp.h"
#include "VoxelHeightGraphStampRef.generated.h"

////////////////////////////////////////////////////
///////// The code below is auto-generated /////////
////////////////////////////////////////////////////

USTRUCT(BlueprintType, DisplayName = "Voxel Height Graph Stamp", Category = "Voxel|Stamp|Height Graph", meta = (HasNativeMake = "/Script/Voxel.VoxelHeightGraphStamp_K2.Make", HasNativeBreak = "/Script/Voxel.VoxelHeightGraphStamp_K2.Break"))
struct VOXEL_API FVoxelHeightGraphStampRef final : public FVoxelHeightStampRef
{
	GENERATED_BODY()
	GENERATED_VOXEL_STAMP_REF_BODY(FVoxelHeightGraphStamp)
};

template<>
struct TStructOpsTypeTraits<FVoxelHeightGraphStampRef> : TStructOpsTypeTraits<FVoxelStampRef>
{
};

template<>
struct TVoxelStampRefImpl<FVoxelHeightGraphStamp>
{
	using Type = FVoxelHeightGraphStampRef;
};