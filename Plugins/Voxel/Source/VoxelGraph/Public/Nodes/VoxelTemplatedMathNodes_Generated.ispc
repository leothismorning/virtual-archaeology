// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelISPCNodeHelpers.isph"

export void VoxelNode_VoxelComputeNode_Frac_Double(const FVoxelInputBuffer* uniform InputBuffers, void* uniform const* uniform OutputBuffers, const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const double Value = LoadDouble(InputBuffers[0], Index);
		double ReturnValue;

		ReturnValue = Value - floor(Value);

		StoreDouble(OutputBuffers[0], Index, ReturnValue);
	}
}

export void VoxelNode_VoxelComputeNode_Frac_Float(const FVoxelInputBuffer* uniform InputBuffers, void* uniform const* uniform OutputBuffers, const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const float Value = LoadFloat(InputBuffers[0], Index);
		float ReturnValue;

		ReturnValue = Value - floor(Value);

		StoreFloat(OutputBuffers[0], Index, ReturnValue);
	}
}

export void VoxelNode_VoxelComputeNode_Power_Double(const FVoxelInputBuffer* uniform InputBuffers, void* uniform const* uniform OutputBuffers, const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const double Base = LoadDouble(InputBuffers[0], Index);
		const double Exp = LoadDouble(InputBuffers[1], Index);
		double ReturnValue;

		ReturnValue = pow(abs(Base), Exp);

		StoreDouble(OutputBuffers[0], Index, ReturnValue);
	}
}

export void VoxelNode_VoxelComputeNode_Power_Float(const FVoxelInputBuffer* uniform InputBuffers, void* uniform const* uniform OutputBuffers, const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const float Base = LoadFloat(InputBuffers[0], Index);
		const float Exp = LoadFloat(InputBuffers[1], Index);
		float ReturnValue;

		ReturnValue = pow(abs(Base), Exp);

		StoreFloat(OutputBuffers[0], Index, ReturnValue);
	}
}