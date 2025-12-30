// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/VoxelSculptGraphFunctionLibrary.h"
#include "VoxelBufferSplitter.h"

void FVoxelGraphParameters::FHeightSculpt::Split(
	const FVoxelBufferSplitter& Splitter,
	const TConstVoxelArrayView<FHeightSculpt*> OutResult) const
{
	VOXEL_FUNCTION_COUNTER();

	{
		TVoxelInlineArray<FVoxelFloatBuffer*, 8> Buffers;
		FVoxelUtilities::SetNumZeroed(Buffers, Splitter.NumOutputs());

		for (const int32 Index : Splitter.GetValidOutputs())
		{
			Buffers[Index] = &OutResult[Index]->PreviousHeights;
		}

		PreviousHeights.Split(Splitter, Buffers);
	}

	{
		TVoxelInlineArray<FVoxelBoolBuffer*, 8> Buffers;
		FVoxelUtilities::SetNumZeroed(Buffers, Splitter.NumOutputs());

		for (const int32 Index : Splitter.GetValidOutputs())
		{
			Buffers[Index] = &OutResult[Index]->IsValid;
		}

		IsValid.Split(Splitter, Buffers);
	}
}

void FVoxelGraphParameters::FVolumeSculpt::Split(
	const FVoxelBufferSplitter& Splitter,
	const TConstVoxelArrayView<FVolumeSculpt*> OutResult) const
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelInlineArray<FVoxelFloatBuffer*, 8> Buffers;
	FVoxelUtilities::SetNumZeroed(Buffers, Splitter.NumOutputs());

	for (const int32 Index : Splitter.GetValidOutputs())
	{
		Buffers[Index] = &OutResult[Index]->PreviousDistances;
	}

	PreviousDistances.Split(Splitter, Buffers);
}

void UVoxelSculptGraphFunctionLibrary::GetPreviousHeight(
	FVoxelFloatBuffer& Height,
	FVoxelBoolBuffer& IsValid) const
{
	if (Query.IsPreview())
	{
		return;
	}

	const FVoxelGraphParameters::FHeightSculpt* Parameter = Query->FindParameter<FVoxelGraphParameters::FHeightSculpt>();
	if (!Parameter)
	{
		VOXEL_MESSAGE(Error, "{0}: No height sculpt data", this);
		return;
	}

	Height = Parameter->PreviousHeights;
	IsValid = Parameter->IsValid;
}

FVoxelFloatBuffer UVoxelSculptGraphFunctionLibrary::GetPreviousDistance() const
{
	if (Query.IsPreview())
	{
		return DefaultBuffer;
	}

	const FVoxelGraphParameters::FVolumeSculpt* Parameter = Query->FindParameter<FVoxelGraphParameters::FVolumeSculpt>();
	if (!Parameter)
	{
		VOXEL_MESSAGE(Error, "{0}: No volume sculpt data", this);
		return DefaultBuffer;
	}

	return Parameter->PreviousDistances;
}