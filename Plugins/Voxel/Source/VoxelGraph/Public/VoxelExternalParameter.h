// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelExternalParameter.generated.h"

USTRUCT()
struct VOXELGRAPH_API FVoxelExternalParameter : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	UPROPERTY()
	FGuid ParameterGuid;
};