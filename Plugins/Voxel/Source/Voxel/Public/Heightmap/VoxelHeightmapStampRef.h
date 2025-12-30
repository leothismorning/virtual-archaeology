// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelHeightStampRef.h"
#include "VoxelHeightmapStamp.h"
#include "VoxelHeightmapStampRef.generated.h"

////////////////////////////////////////////////////
///////// The code below is auto-generated /////////
////////////////////////////////////////////////////

USTRUCT(BlueprintType, DisplayName = "Voxel Heightmap Stamp", Category = "Voxel|Stamp|Heightmap", meta = (HasNativeMake = "/Script/Voxel.VoxelHeightmapStamp_K2.Make", HasNativeBreak = "/Script/Voxel.VoxelHeightmapStamp_K2.Break"))
struct VOXEL_API FVoxelHeightmapStampRef final : public FVoxelHeightStampRef
{
	GENERATED_BODY()
	GENERATED_VOXEL_STAMP_REF_BODY(FVoxelHeightmapStamp)
};

template<>
struct TStructOpsTypeTraits<FVoxelHeightmapStampRef> : TStructOpsTypeTraits<FVoxelStampRef>
{
};

template<>
struct TVoxelStampRefImpl<FVoxelHeightmapStamp>
{
	using Type = FVoxelHeightmapStampRef;
};