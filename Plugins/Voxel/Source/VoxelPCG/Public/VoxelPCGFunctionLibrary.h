// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFunctionLibrary.h"
#include "VoxelPCGFunctionLibrary.generated.h"

namespace FVoxelGraphParameters
{
	struct FPCGBounds : FUniformParameter
	{
		const FVoxelBox Bounds;

		explicit FPCGBounds(const FVoxelBox& Bounds)
			: Bounds(Bounds)
		{
		}
	};
}

UCLASS()
class VOXELPCG_API UVoxelPCGFunctionLibrary : public UVoxelFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(Category = "PCG", DisplayName = "Get PCG Bounds", meta = (AllowList = "PCG"))
	FVoxelBox GetPCGBounds() const;
};