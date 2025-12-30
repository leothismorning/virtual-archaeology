// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelVolumeStampRef.h"
#include "VoxelShapeStamp.h"
#include "VoxelShapeStampRef.generated.h"

////////////////////////////////////////////////////
///////// The code below is auto-generated /////////
////////////////////////////////////////////////////

USTRUCT(BlueprintType, DisplayName = "Voxel Shape Stamp", Category = "Voxel|Stamp|Shape", meta = (HasNativeMake = "/Script/Voxel.VoxelShapeStamp_K2.Make", HasNativeBreak = "/Script/Voxel.VoxelShapeStamp_K2.Break"))
struct VOXEL_API FVoxelShapeStampRef final : public FVoxelVolumeStampRef
{
	GENERATED_BODY()
	GENERATED_VOXEL_STAMP_REF_BODY(FVoxelShapeStamp)
};

template<>
struct TStructOpsTypeTraits<FVoxelShapeStampRef> : TStructOpsTypeTraits<FVoxelStampRef>
{
};

template<>
struct TVoxelStampRefImpl<FVoxelShapeStamp>
{
	using Type = FVoxelShapeStampRef;
};