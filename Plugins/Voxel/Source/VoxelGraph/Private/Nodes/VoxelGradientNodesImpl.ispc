// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.isph"

export void VoxelNode_GetGradient2D_SplitPositions(
	const uniform float PositionX[],
	const uniform float PositionY[],
	const uniform int32 Num,
	const uniform float Granularity,
	uniform float OutPositionX[],
	uniform float OutPositionY[])
{
	const uniform float HalfGranularity = Granularity / 2.f;

	FOREACH(Index, 0, Num)
	{
		IGNORE_PERF_WARNING
		OutPositionX[4 * Index + 0] = PositionX[Index] - HalfGranularity;
		IGNORE_PERF_WARNING
		OutPositionX[4 * Index + 1] = PositionX[Index] + HalfGranularity;
		IGNORE_PERF_WARNING
		OutPositionX[4 * Index + 2] = PositionX[Index];
		IGNORE_PERF_WARNING
		OutPositionX[4 * Index + 3] = PositionX[Index];

		IGNORE_PERF_WARNING
		OutPositionY[4 * Index + 0] = PositionY[Index];
		IGNORE_PERF_WARNING
		OutPositionY[4 * Index + 1] = PositionY[Index];
		IGNORE_PERF_WARNING
		OutPositionY[4 * Index + 2] = PositionY[Index] - HalfGranularity;
		IGNORE_PERF_WARNING
		OutPositionY[4 * Index + 3] = PositionY[Index] + HalfGranularity;
		IGNORE_PERF_WARNING
	}
}

export void VoxelNode_GetGradient2D_Collapse(
	const uniform float Values[],
	const uniform int32 Num,
	const uniform float Granularity,
	uniform float OutX[],
	uniform float OutY[])
{
	FOREACH(Index, 0, Num)
	{
		IGNORE_PERF_WARNING
		OutX[Index] = (Values[4 * Index + 1] - Values[4 * Index + 0]) / Granularity;
		IGNORE_PERF_WARNING
		OutY[Index] = (Values[4 * Index + 3] - Values[4 * Index + 2]) / Granularity;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

export void VoxelNode_GetGradient3D_SplitPositions(
	const uniform float PositionX[],
	const uniform float PositionY[],
	const uniform float PositionZ[],
	const uniform int32 Num,
	const uniform float Granularity,
	uniform float OutPositionX[],
	uniform float OutPositionY[],
	uniform float OutPositionZ[])
{
	const uniform float HalfGranularity = Granularity / 2.f;

	FOREACH(Index, 0, Num)
	{
		IGNORE_PERF_WARNING
		OutPositionX[6 * Index + 0] = PositionX[Index] - HalfGranularity;
		IGNORE_PERF_WARNING
		OutPositionX[6 * Index + 1] = PositionX[Index] + HalfGranularity;
		IGNORE_PERF_WARNING
		OutPositionX[6 * Index + 2] = PositionX[Index];
		IGNORE_PERF_WARNING
		OutPositionX[6 * Index + 3] = PositionX[Index];
		IGNORE_PERF_WARNING
		OutPositionX[6 * Index + 4] = PositionX[Index];
		IGNORE_PERF_WARNING
		OutPositionX[6 * Index + 5] = PositionX[Index];

		IGNORE_PERF_WARNING
		OutPositionY[6 * Index + 0] = PositionY[Index];
		IGNORE_PERF_WARNING
		OutPositionY[6 * Index + 1] = PositionY[Index];
		IGNORE_PERF_WARNING
		OutPositionY[6 * Index + 2] = PositionY[Index] - HalfGranularity;
		IGNORE_PERF_WARNING
		OutPositionY[6 * Index + 3] = PositionY[Index] + HalfGranularity;
		IGNORE_PERF_WARNING
		OutPositionY[6 * Index + 4] = PositionY[Index];
		IGNORE_PERF_WARNING
		OutPositionY[6 * Index + 5] = PositionY[Index];

		IGNORE_PERF_WARNING
		OutPositionZ[6 * Index + 0] = PositionZ[Index];
		IGNORE_PERF_WARNING
		OutPositionZ[6 * Index + 1] = PositionZ[Index];
		IGNORE_PERF_WARNING
		OutPositionZ[6 * Index + 2] = PositionZ[Index];
		IGNORE_PERF_WARNING
		OutPositionZ[6 * Index + 3] = PositionZ[Index];
		IGNORE_PERF_WARNING
		OutPositionZ[6 * Index + 4] = PositionZ[Index] - HalfGranularity;
		IGNORE_PERF_WARNING
		OutPositionZ[6 * Index + 5] = PositionZ[Index] + HalfGranularity;
	}
}

export void VoxelNode_GetGradient3D_Collapse(
	const uniform float Values[],
	const uniform int32 Num,
	const uniform float Granularity,
	uniform float OutX[],
	uniform float OutY[],
	uniform float OutZ[])
{
	FOREACH(Index, 0, Num)
	{
		IGNORE_PERF_WARNING
		OutX[Index] = (Values[6 * Index + 1] - Values[6 * Index + 0]) / Granularity;
		IGNORE_PERF_WARNING
		OutY[Index] = (Values[6 * Index + 3] - Values[6 * Index + 2]) / Granularity;
		IGNORE_PERF_WARNING
		OutZ[Index] = (Values[6 * Index + 5] - Values[6 * Index + 4]) / Granularity;
	}
}