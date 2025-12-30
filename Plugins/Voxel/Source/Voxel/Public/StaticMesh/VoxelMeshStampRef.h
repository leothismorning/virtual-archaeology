// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelVolumeStampRef.h"
#include "VoxelMeshStamp.h"
#include "VoxelMeshStampRef.generated.h"

////////////////////////////////////////////////////
///////// The code below is auto-generated /////////
////////////////////////////////////////////////////

USTRUCT(BlueprintType, DisplayName = "Voxel Mesh Stamp", Category = "Voxel|Stamp|Mesh", meta = (HasNativeMake = "/Script/Voxel.VoxelMeshStamp_K2.Make", HasNativeBreak = "/Script/Voxel.VoxelMeshStamp_K2.Break"))
struct VOXEL_API FVoxelMeshStampRef final : public FVoxelVolumeStampRef
{
	GENERATED_BODY()
	GENERATED_VOXEL_STAMP_REF_BODY(FVoxelMeshStamp)
};

template<>
struct TStructOpsTypeTraits<FVoxelMeshStampRef> : TStructOpsTypeTraits<FVoxelStampRef>
{
};

template<>
struct TVoxelStampRefImpl<FVoxelMeshStamp>
{
	using Type = FVoxelMeshStampRef;
};