// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelISPCNodeHelpers.isph"
#include "VoxelNoiseNodesImpl.isph"

export void VoxelNode_VoxelComputeNode_CellularNoise2D(const FVoxelInputBuffer* uniform InputBuffers, void* uniform const* uniform OutputBuffers, const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const float2 Position = MakeFloat2(LoadFloat(InputBuffers[0], Index), LoadFloat(InputBuffers[1], Index));
		const float Amplitude = LoadFloat(InputBuffers[2], Index);
		const float FeatureScale = LoadFloat(InputBuffers[3], Index);
		const float Jitter = LoadFloat(InputBuffers[4], Index);
		const int32 Seed = LoadInt32(InputBuffers[5], Index);
		float Value;

		Value = GetCellularNoise2D(Seed, Position / FeatureScale, Jitter) * Amplitude;

		StoreFloat(OutputBuffers[0], Index, Value);
	}
}

export void VoxelNode_VoxelComputeNode_CellularNoise3D(const FVoxelInputBuffer* uniform InputBuffers, void* uniform const* uniform OutputBuffers, const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const float3 Position = MakeFloat3(LoadFloat(InputBuffers[0], Index), LoadFloat(InputBuffers[1], Index), LoadFloat(InputBuffers[2], Index));
		const float Amplitude = LoadFloat(InputBuffers[3], Index);
		const float FeatureScale = LoadFloat(InputBuffers[4], Index);
		const float Jitter = LoadFloat(InputBuffers[5], Index);
		const int32 Seed = LoadInt32(InputBuffers[6], Index);
		float Value;

		Value = GetCellularNoise3D(Seed, Position / FeatureScale, Jitter) * Amplitude;

		StoreFloat(OutputBuffers[0], Index, Value);
	}
}

export void VoxelNode_VoxelComputeNode_MixSeeds(const FVoxelInputBuffer* uniform InputBuffers, void* uniform const* uniform OutputBuffers, const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const int32 A = LoadInt32(InputBuffers[0], Index);
		const int32 B = LoadInt32(InputBuffers[1], Index);
		int32 ReturnValue;

		ReturnValue = MurmurHash32(MurmurHash32(A) ^ B);

		StoreInt32(OutputBuffers[0], Index, ReturnValue);
	}
}

export void VoxelNode_VoxelComputeNode_PerlinNoise2D(const FVoxelInputBuffer* uniform InputBuffers, void* uniform const* uniform OutputBuffers, const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const float2 Position = MakeFloat2(LoadFloat(InputBuffers[0], Index), LoadFloat(InputBuffers[1], Index));
		const float Amplitude = LoadFloat(InputBuffers[2], Index);
		const float FeatureScale = LoadFloat(InputBuffers[3], Index);
		const int32 Seed = LoadInt32(InputBuffers[4], Index);
		float Value;

		Value = GetPerlin2D(Seed, Position / FeatureScale) * Amplitude;

		StoreFloat(OutputBuffers[0], Index, Value);
	}
}

export void VoxelNode_VoxelComputeNode_PerlinNoise3D(const FVoxelInputBuffer* uniform InputBuffers, void* uniform const* uniform OutputBuffers, const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const float3 Position = MakeFloat3(LoadFloat(InputBuffers[0], Index), LoadFloat(InputBuffers[1], Index), LoadFloat(InputBuffers[2], Index));
		const float Amplitude = LoadFloat(InputBuffers[3], Index);
		const float FeatureScale = LoadFloat(InputBuffers[4], Index);
		const int32 Seed = LoadInt32(InputBuffers[5], Index);
		float Value;

		Value = GetPerlin3D(Seed, Position / FeatureScale) * Amplitude;

		StoreFloat(OutputBuffers[0], Index, Value);
	}
}

export void VoxelNode_VoxelComputeNode_SimplexNoise2D(const FVoxelInputBuffer* uniform InputBuffers, void* uniform const* uniform OutputBuffers, const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const float2 Position = MakeFloat2(LoadFloat(InputBuffers[0], Index), LoadFloat(InputBuffers[1], Index));
		const float Amplitude = LoadFloat(InputBuffers[2], Index);
		const float FeatureScale = LoadFloat(InputBuffers[3], Index);
		const int32 Seed = LoadInt32(InputBuffers[4], Index);
		float Value;

		Value = GetSimplex2D(Seed, Position / FeatureScale) * Amplitude;

		StoreFloat(OutputBuffers[0], Index, Value);
	}
}

export void VoxelNode_VoxelComputeNode_SimplexNoise3D(const FVoxelInputBuffer* uniform InputBuffers, void* uniform const* uniform OutputBuffers, const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const float3 Position = MakeFloat3(LoadFloat(InputBuffers[0], Index), LoadFloat(InputBuffers[1], Index), LoadFloat(InputBuffers[2], Index));
		const float Amplitude = LoadFloat(InputBuffers[3], Index);
		const float FeatureScale = LoadFloat(InputBuffers[4], Index);
		const int32 Seed = LoadInt32(InputBuffers[5], Index);
		float Value;

		Value = GetSimplex3D(Seed, Position / FeatureScale) * Amplitude;

		StoreFloat(OutputBuffers[0], Index, Value);
	}
}

export void VoxelNode_VoxelComputeNode_TrueDistanceCellularNoise2D(const FVoxelInputBuffer* uniform InputBuffers, void* uniform const* uniform OutputBuffers, const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const float2 Position = MakeFloat2(LoadFloat(InputBuffers[0], Index), LoadFloat(InputBuffers[1], Index));
		const float Amplitude = LoadFloat(InputBuffers[2], Index);
		const float FeatureScale = LoadFloat(InputBuffers[3], Index);
		const float Jitter = LoadFloat(InputBuffers[4], Index);
		const int32 Seed = LoadInt32(InputBuffers[5], Index);
		float Value;
		float2 CellPosition;

		Value = GetTrueDistanceCellularNoise2D(Seed, Position / FeatureScale, Jitter, &CellPosition) * Amplitude;
		CellPosition = CellPosition * FeatureScale;

		StoreFloat(OutputBuffers[0], Index, Value);
		StoreFloat(OutputBuffers[1], Index, CellPosition.x);
		StoreFloat(OutputBuffers[2], Index, CellPosition.y);
	}
}

export void VoxelNode_VoxelComputeNode_TrueDistanceCellularNoise3D(const FVoxelInputBuffer* uniform InputBuffers, void* uniform const* uniform OutputBuffers, const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const float3 Position = MakeFloat3(LoadFloat(InputBuffers[0], Index), LoadFloat(InputBuffers[1], Index), LoadFloat(InputBuffers[2], Index));
		const float Amplitude = LoadFloat(InputBuffers[3], Index);
		const float FeatureScale = LoadFloat(InputBuffers[4], Index);
		const float Jitter = LoadFloat(InputBuffers[5], Index);
		const int32 Seed = LoadInt32(InputBuffers[6], Index);
		float Value;
		float3 CellPosition;

		Value = GetTrueDistanceCellularNoise3D(Seed, Position / FeatureScale, Jitter, &CellPosition) * Amplitude;
		CellPosition = CellPosition * FeatureScale;

		StoreFloat(OutputBuffers[0], Index, Value);
		StoreFloat(OutputBuffers[1], Index, CellPosition.x);
		StoreFloat(OutputBuffers[2], Index, CellPosition.y);
		StoreFloat(OutputBuffers[3], Index, CellPosition.z);
	}
}

export void VoxelNode_VoxelComputeNode_ValueNoise2D(const FVoxelInputBuffer* uniform InputBuffers, void* uniform const* uniform OutputBuffers, const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const float2 Position = MakeFloat2(LoadFloat(InputBuffers[0], Index), LoadFloat(InputBuffers[1], Index));
		const float Amplitude = LoadFloat(InputBuffers[2], Index);
		const float FeatureScale = LoadFloat(InputBuffers[3], Index);
		const int32 Seed = LoadInt32(InputBuffers[4], Index);
		float Value;

		Value = GetValue2D(Seed, Position / FeatureScale) * Amplitude;

		StoreFloat(OutputBuffers[0], Index, Value);
	}
}

export void VoxelNode_VoxelComputeNode_ValueNoise3D(const FVoxelInputBuffer* uniform InputBuffers, void* uniform const* uniform OutputBuffers, const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const float3 Position = MakeFloat3(LoadFloat(InputBuffers[0], Index), LoadFloat(InputBuffers[1], Index), LoadFloat(InputBuffers[2], Index));
		const float Amplitude = LoadFloat(InputBuffers[3], Index);
		const float FeatureScale = LoadFloat(InputBuffers[4], Index);
		const int32 Seed = LoadInt32(InputBuffers[5], Index);
		float Value;

		Value = GetValue3D(Seed, Position / FeatureScale) * Amplitude;

		StoreFloat(OutputBuffers[0], Index, Value);
	}
}