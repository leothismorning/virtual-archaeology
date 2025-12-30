// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelPinValue.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoxelPinValueBlueprintLibrary.generated.h"

UCLASS()
class VOXELGRAPH_API UVoxelPinValueBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, DisplayName = "Make Voxel Pin Value", CustomThunk, Category = "Voxel", meta = (CustomStructureParam = "Value", BlueprintInternalUseOnly = "true", NativeMakeFunc))
	static FVoxelPinValue K2_MakeVoxelPinValue(int32 Value)
	{
		unimplemented();
		return {};
	}
	DECLARE_FUNCTION(execK2_MakeVoxelPinValue);
};