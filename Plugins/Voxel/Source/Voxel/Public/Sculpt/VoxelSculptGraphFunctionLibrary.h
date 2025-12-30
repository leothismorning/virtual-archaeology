// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFunctionLibrary.h"
#include "Buffer/VoxelBaseBuffers.h"
#include "VoxelSculptGraphFunctionLibrary.generated.h"

namespace FVoxelGraphParameters
{
	struct VOXEL_API FHeightSculpt : FBufferParameter
	{
		FVoxelFloatBuffer PreviousHeights;
		FVoxelBoolBuffer IsValid;

		void Split(
			const FVoxelBufferSplitter& Splitter,
			TConstVoxelArrayView<FHeightSculpt*> OutResult) const;
	};
	struct VOXEL_API FVolumeSculpt : FBufferParameter
	{
		FVoxelFloatBuffer PreviousDistances;

		void Split(
			const FVoxelBufferSplitter& Splitter,
			TConstVoxelArrayView<FVolumeSculpt*> OutResult) const;
	};
}

UCLASS()
class VOXEL_API UVoxelSculptGraphFunctionLibrary : public UVoxelFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(Category = "Sculpt", meta = (AllowList = "HeightSculpt"))
	void GetPreviousHeight(
		FVoxelFloatBuffer& Height,
		FVoxelBoolBuffer& IsValid) const;

	UFUNCTION(Category = "Sculpt", meta = (AllowList = "VolumeSculpt"))
	FVoxelFloatBuffer GetPreviousDistance() const;
};