// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.isph"
#include "VoxelStampQueryImpl.isph"
#include "VoxelStampTransformImpl.isph"

// ReSharper disable CppCStyleCast

FORCEINLINE varying float SampleMeshDistances(
	varying float LocalPositionX,
	varying float LocalPositionY,
	varying float LocalPositionZ,
	const uniform float MaxPositionX,
	const uniform float MaxPositionY,
	const uniform float MaxPositionZ,
	const uniform int SizeX,
	const uniform int SizeXY,
	const uniform int SizeXYZ,
	const uniform int3& Size,
	const uniform bool bUseTricubic,
	const uniform float MeshDistances[])
{
	varying float DistanceOffset = 0.f;
	DistanceOffset += Square(max(0.f, 1.f - LocalPositionX));
	DistanceOffset += Square(max(0.f, 1.f - LocalPositionY));
	DistanceOffset += Square(max(0.f, 1.f - LocalPositionZ));
	DistanceOffset += Square(max(0.f, LocalPositionX - MaxPositionX));
	DistanceOffset += Square(max(0.f, LocalPositionY - MaxPositionY));
	DistanceOffset += Square(max(0.f, LocalPositionZ - MaxPositionZ));

	LocalPositionX = clamp(LocalPositionX, 1.f, MaxPositionX);
	LocalPositionY = clamp(LocalPositionY, 1.f, MaxPositionY);
	LocalPositionZ = clamp(LocalPositionZ, 1.f, MaxPositionZ);

	DistanceOffset = sqrt(DistanceOffset);

	check(IsFinite(DistanceOffset));

	const varying float MinXf = floor(LocalPositionX);
	const varying float MinYf = floor(LocalPositionY);
	const varying float MinZf = floor(LocalPositionZ);

	const varying float AlphaX = LocalPositionX - MinXf;
	const varying float AlphaY = LocalPositionY - MinYf;
	const varying float AlphaZ = LocalPositionZ - MinZf;

	check(1 <= MinXf && MinXf + 2 < Size.x);
	check(1 <= MinYf && MinYf + 2 < Size.y);
	check(1 <= MinZf && MinZf + 2 < Size.z);

	const varying int32 MinX = (int32)MinXf;
	const varying int32 MinY = (int32)MinYf;
	const varying int32 MinZ = (int32)MinZf;

	if (!bUseTricubic)
	{
		return DistanceOffset + TrilinearInterpolation(
			IGNORE_PERF_WARNING
			MeshDistances[(MinX + 0) + SizeX * (MinY + 0) + SizeXY * (MinZ + 0)],
			IGNORE_PERF_WARNING
			MeshDistances[(MinX + 1) + SizeX * (MinY + 0) + SizeXY * (MinZ + 0)],
			IGNORE_PERF_WARNING
			MeshDistances[(MinX + 0) + SizeX * (MinY + 1) + SizeXY * (MinZ + 0)],
			IGNORE_PERF_WARNING
			MeshDistances[(MinX + 1) + SizeX * (MinY + 1) + SizeXY * (MinZ + 0)],
			IGNORE_PERF_WARNING
			MeshDistances[(MinX + 0) + SizeX * (MinY + 0) + SizeXY * (MinZ + 1)],
			IGNORE_PERF_WARNING
			MeshDistances[(MinX + 1) + SizeX * (MinY + 0) + SizeXY * (MinZ + 1)],
			IGNORE_PERF_WARNING
			MeshDistances[(MinX + 0) + SizeX * (MinY + 1) + SizeXY * (MinZ + 1)],
			IGNORE_PERF_WARNING
			MeshDistances[(MinX + 1) + SizeX * (MinY + 1) + SizeXY * (MinZ + 1)],
			AlphaX,
			AlphaY,
			AlphaZ);
	}

#if 0
	return DistanceOffset + TricubicInterpolation(
		IGNORE_PERF_WARNING
		MeshDistances[(MinX - 1) + SizeX * (MinY - 1) + SizeXY * (MinZ - 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 0) + SizeX * (MinY - 1) + SizeXY * (MinZ - 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 1) + SizeX * (MinY - 1) + SizeXY * (MinZ - 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 2) + SizeX * (MinY - 1) + SizeXY * (MinZ - 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX - 1) + SizeX * (MinY + 0) + SizeXY * (MinZ - 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 0) + SizeX * (MinY + 0) + SizeXY * (MinZ - 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 1) + SizeX * (MinY + 0) + SizeXY * (MinZ - 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 2) + SizeX * (MinY + 0) + SizeXY * (MinZ - 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX - 1) + SizeX * (MinY + 1) + SizeXY * (MinZ - 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 0) + SizeX * (MinY + 1) + SizeXY * (MinZ - 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 1) + SizeX * (MinY + 1) + SizeXY * (MinZ - 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 2) + SizeX * (MinY + 1) + SizeXY * (MinZ - 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX - 1) + SizeX * (MinY + 2) + SizeXY * (MinZ - 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 0) + SizeX * (MinY + 2) + SizeXY * (MinZ - 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 1) + SizeX * (MinY + 2) + SizeXY * (MinZ - 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 2) + SizeX * (MinY + 2) + SizeXY * (MinZ - 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX - 1) + SizeX * (MinY - 1) + SizeXY * (MinZ + 0)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 0) + SizeX * (MinY - 1) + SizeXY * (MinZ + 0)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 1) + SizeX * (MinY - 1) + SizeXY * (MinZ + 0)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 2) + SizeX * (MinY - 1) + SizeXY * (MinZ + 0)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX - 1) + SizeX * (MinY + 0) + SizeXY * (MinZ + 0)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 0) + SizeX * (MinY + 0) + SizeXY * (MinZ + 0)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 1) + SizeX * (MinY + 0) + SizeXY * (MinZ + 0)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 2) + SizeX * (MinY + 0) + SizeXY * (MinZ + 0)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX - 1) + SizeX * (MinY + 1) + SizeXY * (MinZ + 0)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 0) + SizeX * (MinY + 1) + SizeXY * (MinZ + 0)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 1) + SizeX * (MinY + 1) + SizeXY * (MinZ + 0)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 2) + SizeX * (MinY + 1) + SizeXY * (MinZ + 0)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX - 1) + SizeX * (MinY + 2) + SizeXY * (MinZ + 0)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 0) + SizeX * (MinY + 2) + SizeXY * (MinZ + 0)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 1) + SizeX * (MinY + 2) + SizeXY * (MinZ + 0)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 2) + SizeX * (MinY + 2) + SizeXY * (MinZ + 0)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX - 1) + SizeX * (MinY - 1) + SizeXY * (MinZ + 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 0) + SizeX * (MinY - 1) + SizeXY * (MinZ + 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 1) + SizeX * (MinY - 1) + SizeXY * (MinZ + 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 2) + SizeX * (MinY - 1) + SizeXY * (MinZ + 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX - 1) + SizeX * (MinY + 0) + SizeXY * (MinZ + 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 0) + SizeX * (MinY + 0) + SizeXY * (MinZ + 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 1) + SizeX * (MinY + 0) + SizeXY * (MinZ + 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 2) + SizeX * (MinY + 0) + SizeXY * (MinZ + 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX - 1) + SizeX * (MinY + 1) + SizeXY * (MinZ + 0)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 0) + SizeX * (MinY + 1) + SizeXY * (MinZ + 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 1) + SizeX * (MinY + 1) + SizeXY * (MinZ + 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 2) + SizeX * (MinY + 1) + SizeXY * (MinZ + 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX - 1) + SizeX * (MinY + 2) + SizeXY * (MinZ + 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 0) + SizeX * (MinY + 2) + SizeXY * (MinZ + 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 1) + SizeX * (MinY + 2) + SizeXY * (MinZ + 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 2) + SizeX * (MinY + 2) + SizeXY * (MinZ + 1)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX - 1) + SizeX * (MinY - 1) + SizeXY * (MinZ + 2)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 0) + SizeX * (MinY - 1) + SizeXY * (MinZ + 2)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 1) + SizeX * (MinY - 1) + SizeXY * (MinZ + 2)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 2) + SizeX * (MinY - 1) + SizeXY * (MinZ + 2)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX - 1) + SizeX * (MinY + 0) + SizeXY * (MinZ + 2)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 0) + SizeX * (MinY + 0) + SizeXY * (MinZ + 2)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 1) + SizeX * (MinY + 0) + SizeXY * (MinZ + 2)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 2) + SizeX * (MinY + 0) + SizeXY * (MinZ + 2)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX - 1) + SizeX * (MinY + 1) + SizeXY * (MinZ + 2)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 0) + SizeX * (MinY + 1) + SizeXY * (MinZ + 2)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 1) + SizeX * (MinY + 1) + SizeXY * (MinZ + 2)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 2) + SizeX * (MinY + 1) + SizeXY * (MinZ + 2)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX - 1) + SizeX * (MinY + 2) + SizeXY * (MinZ + 2)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 0) + SizeX * (MinY + 2) + SizeXY * (MinZ + 2)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 1) + SizeX * (MinY + 2) + SizeXY * (MinZ + 2)],
		IGNORE_PERF_WARNING
		MeshDistances[(MinX + 2) + SizeX * (MinY + 2) + SizeXY * (MinZ + 2)],
		AlphaX,
		AlphaY,
		AlphaZ);
#endif

	const varying float p0_Y_MultiplierVector = ((-0.5f * AlphaY + 1.0f) * AlphaY - 0.5f) * AlphaY + 0.0f;
	const varying float p1_Y_MultiplierVector = ((+1.5f * AlphaY - 2.5f) * AlphaY - 0.0f) * AlphaY + 1.0f;
	const varying float p2_Y_MultiplierVector = ((-1.5f * AlphaY + 2.0f) * AlphaY + 0.5f) * AlphaY + 0.0f;
	const varying float p3_Y_MultiplierVector = ((+0.5f * AlphaY - 0.5f) * AlphaY + 0.0f) * AlphaY + 0.0f;

	const varying float p0_Z_MultiplierVector = ((-0.5f * AlphaZ + 1.0f) * AlphaZ - 0.5f) * AlphaZ + 0.0f;
	const varying float p1_Z_MultiplierVector = ((+1.5f * AlphaZ - 2.5f) * AlphaZ - 0.0f) * AlphaZ + 1.0f;
	const varying float p2_Z_MultiplierVector = ((-1.5f * AlphaZ + 2.0f) * AlphaZ + 0.5f) * AlphaZ + 0.0f;
	const varying float p3_Z_MultiplierVector = ((+0.5f * AlphaZ - 0.5f) * AlphaZ + 0.0f) * AlphaZ + 0.0f;

	const varying int32 BaseIndexVector = (MinX - 1) + SizeX * (MinY - 1) + SizeXY * (MinZ - 1);

	varying float Result;

	// Read the current active lanes
	const uniform int64 LaneMask = lanemask_unsafe();

	// Reset active lanes
	unmasked_unsafe
	{
		UNROLL_N(TARGET_WIDTH)
		for (uniform int32 Lane = 0; Lane < programCount; Lane++)
		{
			if (!(LaneMask & (1u << Lane)))
			{
				continue;
			}

			const uniform int32 BaseIndex = extract(BaseIndexVector, Lane);
			const uniform int32 LaneMinX = extract(MinX, Lane);
			const uniform int32 LaneMinY = extract(MinY, Lane);
			const uniform int32 LaneMinZ = extract(MinZ, Lane);
			const uniform float LaneAlphaX = extract(AlphaX, Lane);
			const uniform float p0_Y_Multiplier = extract(p0_Y_MultiplierVector, Lane);
			const uniform float p1_Y_Multiplier = extract(p1_Y_MultiplierVector, Lane);
			const uniform float p2_Y_Multiplier = extract(p2_Y_MultiplierVector, Lane);
			const uniform float p3_Y_Multiplier = extract(p3_Y_MultiplierVector, Lane);
			const uniform float p0_Z_Multiplier = extract(p0_Z_MultiplierVector, Lane);
			const uniform float p1_Z_Multiplier = extract(p1_Z_MultiplierVector, Lane);
			const uniform float p2_Z_Multiplier = extract(p2_Z_MultiplierVector, Lane);
			const uniform float p3_Z_Multiplier = extract(p3_Z_MultiplierVector, Lane);

			check(0 <= LaneMinX - 1 && LaneMinX + 2 < Size.x);
			check(0 <= LaneMinY - 1 && LaneMinY + 2 < Size.y);
			check(0 <= LaneMinZ - 1 && LaneMinZ + 2 < Size.z);

			check(BaseIndex + 2 * SizeX + 3 * SizeXY + TARGET_WIDTH <= SizeXYZ);
#if TARGET_WIDTH == 8
			check(BaseIndex + 3 * SizeX + 3 * SizeXY - 4 + TARGET_WIDTH <= SizeXYZ);
#elif TARGET_WIDTH == 4
			check(BaseIndex + 3 * SizeX + 3 * SizeXY + TARGET_WIDTH <= SizeXYZ);
#endif

			const varying float A = MakeRegister4_ZeroPadded(+0.0f, +1.0f, +0.0f, +0.0f);
			const varying float B = MakeRegister4_ZeroPadded(-0.5f, -0.0f, +0.5f, +0.0f);
			const varying float C = MakeRegister4_ZeroPadded(+1.0f, -2.5f, +2.0f, -0.5f);
			const varying float D = MakeRegister4_ZeroPadded(-0.5f, +1.5f, -1.5f, +0.5f);

			const varying float p0123_X_Multiplier = A + LaneAlphaX * (B + LaneAlphaX * (C + LaneAlphaX * D));

			check(BaseIndex + 2 * SizeX + 3 * SizeXY + TARGET_WIDTH <= SizeXYZ);
			check(BaseIndex + 3 * SizeX + 3 * SizeXY - 4 + TARGET_WIDTH <= SizeXYZ);

			const uniform float Value = reduce_add(
				p0123_X_Multiplier * (
					p0_Z_Multiplier * (
						p0_Y_Multiplier * MeshDistances[BaseIndex + 0 * SizeX + 0 * SizeXY + programIndex] +
						p1_Y_Multiplier * MeshDistances[BaseIndex + 1 * SizeX + 0 * SizeXY + programIndex] +
						p2_Y_Multiplier * MeshDistances[BaseIndex + 2 * SizeX + 0 * SizeXY + programIndex] +
						p3_Y_Multiplier * MeshDistances[BaseIndex + 3 * SizeX + 0 * SizeXY + programIndex]) +

					p1_Z_Multiplier * (
						p0_Y_Multiplier * MeshDistances[BaseIndex + 0 * SizeX + 1 * SizeXY + programIndex] +
						p1_Y_Multiplier * MeshDistances[BaseIndex + 1 * SizeX + 1 * SizeXY + programIndex] +
						p2_Y_Multiplier * MeshDistances[BaseIndex + 2 * SizeX + 1 * SizeXY + programIndex] +
						p3_Y_Multiplier * MeshDistances[BaseIndex + 3 * SizeX + 1 * SizeXY + programIndex]) +

					p2_Z_Multiplier * (
						p0_Y_Multiplier * MeshDistances[BaseIndex + 0 * SizeX + 2 * SizeXY + programIndex] +
						p1_Y_Multiplier * MeshDistances[BaseIndex + 1 * SizeX + 2 * SizeXY + programIndex] +
						p2_Y_Multiplier * MeshDistances[BaseIndex + 2 * SizeX + 2 * SizeXY + programIndex] +
						p3_Y_Multiplier * MeshDistances[BaseIndex + 3 * SizeX + 2 * SizeXY + programIndex]) +

					p3_Z_Multiplier * (
						p0_Y_Multiplier * MeshDistances[BaseIndex + 0 * SizeX + 3 * SizeXY + programIndex] +
						p1_Y_Multiplier * MeshDistances[BaseIndex + 1 * SizeX + 3 * SizeXY + programIndex] +
						p2_Y_Multiplier * MeshDistances[BaseIndex + 2 * SizeX + 3 * SizeXY + programIndex] +
#if TARGET_WIDTH == 8
						// Don't access out of bounds
						p3_Y_Multiplier * rotate(MeshDistances[BaseIndex + 3 * SizeX + 3 * SizeXY - 4 + programIndex], 4))));
#elif TARGET_WIDTH == 4
						p3_Y_Multiplier * MeshDistances[BaseIndex + 3 * SizeX + 3 * SizeXY + programIndex])));
#endif

			Result = insert(Result, Lane, Value);
		}
	}

	return DistanceOffset + Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

export void VoxelMeshStamp_ApplyGrid(
	const uniform FVoxelVolumeBulkQuery& Query,
	const uniform FVoxelVolumeTransform& StampToQuery,
	const uniform EVoxelVolumeBlendMode BlendMode,
	const uniform bool bApplyOnVoid,
	const uniform float Smoothness,
	const uniform bool bUseTricubic,
	const uniform int3& Size,
	const uniform float VoxelSize,
	const uniform float3& MeshOrigin,
	const uniform float MeshDistances[])
{
	const uniform float MaxPositionX = Size.x - 2.0001f;
	const uniform float MaxPositionY = Size.y - 2.0001f;
	const uniform float MaxPositionZ = Size.z - 2.0001f;

	const uniform float InvVoxelSize = 1.f / VoxelSize;

	const uniform int32 SizeX = Size.x;
	const uniform int32 SizeXY = Size.x * Size.y;
	const uniform int32 SizeXYZ = Size.x * Size.y * Size.z;

	for (uniform int32 IndexZ = Query.StartIndex.z; IndexZ < Query.EndIndex.z; IndexZ++)
	{
		for (uniform int32 IndexY = Query.StartIndex.y; IndexY < Query.EndIndex.y; IndexY++)
		{
			FOREACH(IndexX, Query.StartIndex.x, Query.EndIndex.x)
			{
				varying float3 LocalPosition = InverseTransform(StampToQuery, GetPosition(Query, IndexX, IndexY, IndexZ));
				LocalPosition = LocalPosition * InvVoxelSize - MeshOrigin;

				varying float NewDistance = SampleMeshDistances(
					LocalPosition.x,
					LocalPosition.y,
					LocalPosition.z,
					MaxPositionX,
					MaxPositionY,
					MaxPositionZ,
					SizeX,
					SizeXY,
					SizeXYZ,
					Size,
					bUseTricubic,
					MeshDistances);

				NewDistance *= VoxelSize;
				NewDistance = TransformDistance(StampToQuery, NewDistance);

				coherent_if (intbits(NewDistance) == NaNf_uint)
				{
					continue;
				}

				const varying int32 Index = GetIndex(Query, IndexX, IndexY, IndexZ);
				const varying float OldDistance = Query.Distances[Index];

				switch (BlendMode)
				{
				default: VOXEL_ASSUME(false);
				case VolumeBlendMode_Additive:
				{
					if (intbits(OldDistance) == NaNf_uint)
					{
						if (bApplyOnVoid)
						{
							Query.Distances[Index] = NewDistance;
						}
					}
					else
					{
						Query.Distances[Index] = SmoothMin(
							OldDistance,
							NewDistance,
							Smoothness);
					}
				}
				break;
				case VolumeBlendMode_Subtractive:
				{
					if (intbits(OldDistance) == NaNf_uint)
					{
						if (bApplyOnVoid)
						{
							Query.Distances[Index] = -NewDistance;
						}
					}
					else
					{
						Query.Distances[Index] = SmoothMax(
							OldDistance,
							-NewDistance,
							Smoothness);
					}
				}
				break;
				case VolumeBlendMode_Intersect:
				{
					if (intbits(OldDistance) != NaNf_uint)
					{
						Query.Distances[Index] = SmoothMax(
							OldDistance,
							NewDistance,
							Smoothness);
					}
				}
				break;
				case VolumeBlendMode_Override:
				{
					Query.Distances[Index] = NewDistance;
				}
				break;
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

export void VoxelMeshStamp_ApplySparse(
	const uniform FVoxelVolumeSparseQuery& Query,
	const uniform FVoxelVolumeTransform& StampToQuery,
	uniform float Alphas[],
	const uniform bool bWriteDistance,
	const uniform bool bWriteAlpha,
	const uniform EVoxelVolumeBlendMode BlendMode,
	const uniform bool bApplyOnVoid,
	const uniform float Smoothness,
	const uniform bool bUseTricubic,
	const uniform int3& Size,
	const uniform float VoxelSize,
	const uniform float3& MeshOrigin,
	const uniform float MeshDistances[])
{
	check(bWriteDistance || bWriteAlpha);

	const uniform float MaxPositionX = Size.x - 2.0001f;
	const uniform float MaxPositionY = Size.y - 2.0001f;
	const uniform float MaxPositionZ = Size.z - 2.0001f;

	const uniform float InvVoxelSize = 1.f / VoxelSize;

	const uniform int32 SizeX = Size.x;
	const uniform int32 SizeXY = Size.x * Size.y;
	const uniform int32 SizeXYZ = Size.x * Size.y * Size.z;

	FOREACH(Index, 0, Query.Num)
	{
		const varying double3 WorldPosition = MakeDouble3(
			Query.PositionsX[Index],
			Query.PositionsY[Index],
			Query.PositionsZ[Index]);

		varying float3 LocalPosition = InverseTransform(StampToQuery, WorldPosition);
		LocalPosition = LocalPosition * InvVoxelSize - MeshOrigin;

		varying float NewDistance = SampleMeshDistances(
			LocalPosition.x,
			LocalPosition.y,
			LocalPosition.z,
			MaxPositionX,
			MaxPositionY,
			MaxPositionZ,
			SizeX,
			SizeXY,
			SizeXYZ,
			Size,
			bUseTricubic,
			MeshDistances);

		NewDistance *= VoxelSize;
		NewDistance = TransformDistance(StampToQuery, NewDistance);

		coherent_if (intbits(NewDistance) == NaNf_uint)
		{
			if (bWriteAlpha)
			{
				Alphas[Index] = 0.f;
			}
			continue;
		}

		const varying float OldDistance = GetDistance(Query, Index);

		switch (BlendMode)
		{
		default: VOXEL_ASSUME(false);
		case VolumeBlendMode_Additive:
		{
			if (intbits(OldDistance) == NaNf_uint)
			{
				if (bApplyOnVoid)
				{
					if (bWriteDistance)
					{
						SetDistance(Query, Index, NewDistance);
					}

					if (bWriteAlpha)
					{
						Alphas[Index] = 1.f;
					}
				}
				else
				{
					if (bWriteAlpha)
					{
						Alphas[Index] = 0.f;
					}
				}
			}
			else
			{
				const varying float Alpha = GetSmoothMinAlpha(OldDistance, NewDistance, Smoothness);

				if (bWriteDistance)
				{
					SetDistance(Query, Index, SmoothMinFromAlpha(
						OldDistance,
						NewDistance,
						Smoothness,
						Alpha));
				}

				if (bWriteAlpha)
				{
					Alphas[Index] = Alpha;
				}
			}
		}
		break;
		case VolumeBlendMode_Subtractive:
		{
			if (intbits(OldDistance) == NaNf_uint)
			{
				if (bApplyOnVoid)
				{
					if (bWriteDistance)
					{
						SetDistance(Query, Index, -NewDistance);
					}

					if (bWriteAlpha)
					{
						Alphas[Index] = 1.f;
					}
				}
				else
				{
					if (bWriteAlpha)
					{
						Alphas[Index] = 0.f;
					}
				}
			}
			else
			{
				const varying float Alpha = GetSmoothMaxAlpha(OldDistance, -NewDistance, Smoothness);

				if (bWriteDistance)
				{
					SetDistance(Query, Index, SmoothMaxFromAlpha(
						OldDistance,
						-NewDistance,
						Smoothness,
						Alpha));
				}

				if (bWriteAlpha)
				{
					Alphas[Index] = Alpha;
				}
			}
		}
		break;
		case VolumeBlendMode_Intersect:
		{
			if (intbits(OldDistance) == NaNf_uint)
			{
				if (bWriteAlpha)
				{
					Alphas[Index] = 0.f;
				}
			}
			else
			{
				const varying float Alpha = GetSmoothMaxAlpha(OldDistance, NewDistance, Smoothness);

				if (bWriteDistance)
				{
					SetDistance(Query, Index, SmoothMaxFromAlpha(
						OldDistance,
						NewDistance,
						Smoothness,
						Alpha));
				}

				if (bWriteAlpha)
				{
					Alphas[Index] = Alpha;
				}
			}
		}
		break;
		case VolumeBlendMode_Override:
		{
			if (bWriteDistance)
			{
				SetDistance(Query, Index, NewDistance);
			}

			if (bWriteAlpha)
			{
				Alphas[Index] = 1.f;
			}
		}
		break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

export void VoxelMeshStamp_ApplyMetadata_Float(
	const uniform FVoxelVolumeSparseQuery& Query,
	const uniform FVoxelVolumeTransform& StampToQuery,
	uniform float InOutIndirectValues[],
	const uniform float Alphas[],
	const uniform int3& Size,
	const uniform float VoxelSize,
	const uniform float3& MeshOrigin,
	const uniform int32 Indices[],
	const uniform float MeshValues[])
{
	const uniform float MaxPositionX = Size.x - 2.0001;
	const uniform float MaxPositionY = Size.y - 2.0001;
	const uniform float MaxPositionZ = Size.z - 2.0001;

	const uniform float InvVoxelSize = 1.f / VoxelSize;

	const uniform int32 SizeX = Size.x;
	const uniform int32 SizeXY = Size.x * Size.y;

	FOREACH(Index, 0, Query.Num)
	{
		const varying float Alpha = Alphas[Index];
		coherent_if (Alpha == 0.f)
		{
			continue;
		}

		const varying double3 WorldPosition = MakeDouble3(
			Query.PositionsX[Index],
			Query.PositionsY[Index],
			Query.PositionsZ[Index]);

		varying float3 LocalPosition = InverseTransform(StampToQuery, WorldPosition);
		LocalPosition = LocalPosition * InvVoxelSize - MeshOrigin;

		LocalPosition.x = clamp(LocalPosition.x, 1.f, MaxPositionX);
		LocalPosition.y = clamp(LocalPosition.y, 1.f, MaxPositionY);
		LocalPosition.z = clamp(LocalPosition.z, 1.f, MaxPositionZ);

		const varying float MinXf = floor(LocalPosition.x);
		const varying float MinYf = floor(LocalPosition.y);
		const varying float MinZf = floor(LocalPosition.z);

		const varying float AlphaX = LocalPosition.x - MinXf;
		const varying float AlphaY = LocalPosition.y - MinYf;
		const varying float AlphaZ = LocalPosition.z - MinZf;

		const varying int32 MinX = (int32)MinXf;
		const varying int32 MinY = (int32)MinYf;
		const varying int32 MinZ = (int32)MinZf;

		check(0 <= MinX && MinX + 1 < Size.x);
		check(0 <= MinY && MinY + 1 < Size.y);
		check(0 <= MinZ && MinZ + 1 < Size.z);

		IGNORE_PERF_WARNING
		const int32 Index000 = Indices[(MinX + 0) + SizeX * (MinY + 0) + SizeXY * (MinZ + 0)];
		IGNORE_PERF_WARNING
		const int32 Index001 = Indices[(MinX + 1) + SizeX * (MinY + 0) + SizeXY * (MinZ + 0)];
		IGNORE_PERF_WARNING
		const int32 Index010 = Indices[(MinX + 0) + SizeX * (MinY + 1) + SizeXY * (MinZ + 0)];
		IGNORE_PERF_WARNING
		const int32 Index011 = Indices[(MinX + 1) + SizeX * (MinY + 1) + SizeXY * (MinZ + 0)];
		IGNORE_PERF_WARNING
		const int32 Index100 = Indices[(MinX + 0) + SizeX * (MinY + 0) + SizeXY * (MinZ + 1)];
		IGNORE_PERF_WARNING
		const int32 Index101 = Indices[(MinX + 1) + SizeX * (MinY + 0) + SizeXY * (MinZ + 1)];
		IGNORE_PERF_WARNING
		const int32 Index110 = Indices[(MinX + 0) + SizeX * (MinY + 1) + SizeXY * (MinZ + 1)];
		IGNORE_PERF_WARNING
		const int32 Index111 = Indices[(MinX + 1) + SizeX * (MinY + 1) + SizeXY * (MinZ + 1)];

		float OldValue;
		if (Query.Indirection)
		{
			IGNORE_PERF_WARNING
			OldValue = InOutIndirectValues[Query.Indirection[Index]];
		}
		else
		{
			IGNORE_PERF_WARNING
			OldValue = InOutIndirectValues[Index];
		}

		const float NewValue = TrilinearInterpolation(
			IGNORE_PERF_WARNING
			MeshValues[Index000],
			IGNORE_PERF_WARNING
			MeshValues[Index001],
			IGNORE_PERF_WARNING
			MeshValues[Index010],
			IGNORE_PERF_WARNING
			MeshValues[Index011],
			IGNORE_PERF_WARNING
			MeshValues[Index100],
			IGNORE_PERF_WARNING
			MeshValues[Index101],
			IGNORE_PERF_WARNING
			MeshValues[Index110],
			IGNORE_PERF_WARNING
			MeshValues[Index111],
			AlphaX,
			AlphaY,
			AlphaZ);

		if (Query.Indirection)
		{
			IGNORE_PERF_WARNING
			InOutIndirectValues[Query.Indirection[Index]] = lerp(OldValue, NewValue, Alpha);
		}
		else
		{
			IGNORE_PERF_WARNING
			InOutIndirectValues[Index] = lerp(OldValue, NewValue, Alpha);
		}
	}
}

export void VoxelMeshStamp_ApplyMetadata_LinearColor(
	const uniform FVoxelVolumeSparseQuery& Query,
	const uniform FVoxelVolumeTransform& StampToQuery,
	uniform float InOutIndirectValuesR[],
	uniform float InOutIndirectValuesG[],
	uniform float InOutIndirectValuesB[],
	uniform float InOutIndirectValuesA[],
	const uniform float Alphas[],
	const uniform int3& Size,
	const uniform float VoxelSize,
	const uniform float3& MeshOrigin,
	const uniform int32 Indices[],
	const uniform float MeshValuesR[],
	const uniform float MeshValuesG[],
	const uniform float MeshValuesB[],
	const uniform float MeshValuesA[])
{
	const uniform float MaxPositionX = Size.x - 2.0001;
	const uniform float MaxPositionY = Size.y - 2.0001;
	const uniform float MaxPositionZ = Size.z - 2.0001;

	const uniform float InvVoxelSize = 1.f / VoxelSize;

	const uniform int32 SizeX = Size.x;
	const uniform int32 SizeXY = Size.x * Size.y;

	FOREACH(Index, 0, Query.Num)
	{
		const varying float Alpha = Alphas[Index];
		coherent_if (Alpha == 0.f)
		{
			continue;
		}

		const varying double3 WorldPosition = MakeDouble3(
			Query.PositionsX[Index],
			Query.PositionsY[Index],
			Query.PositionsZ[Index]);

		varying float3 LocalPosition = InverseTransform(StampToQuery, WorldPosition);
		LocalPosition = LocalPosition * InvVoxelSize - MeshOrigin;

		LocalPosition.x = clamp(LocalPosition.x, 1.f, MaxPositionX);
		LocalPosition.y = clamp(LocalPosition.y, 1.f, MaxPositionY);
		LocalPosition.z = clamp(LocalPosition.z, 1.f, MaxPositionZ);

		const varying float MinXf = floor(LocalPosition.x);
		const varying float MinYf = floor(LocalPosition.y);
		const varying float MinZf = floor(LocalPosition.z);

		const varying float AlphaX = LocalPosition.x - MinXf;
		const varying float AlphaY = LocalPosition.y - MinYf;
		const varying float AlphaZ = LocalPosition.z - MinZf;

		const varying int32 MinX = (int32)MinXf;
		const varying int32 MinY = (int32)MinYf;
		const varying int32 MinZ = (int32)MinZf;

		check(0 <= MinX && MinX + 1 < Size.x);
		check(0 <= MinY && MinY + 1 < Size.y);
		check(0 <= MinZ && MinZ + 1 < Size.z);

		IGNORE_PERF_WARNING
		const int32 Index000 = Indices[(MinX + 0) + SizeX * (MinY + 0) + SizeXY * (MinZ + 0)];
		IGNORE_PERF_WARNING
		const int32 Index001 = Indices[(MinX + 1) + SizeX * (MinY + 0) + SizeXY * (MinZ + 0)];
		IGNORE_PERF_WARNING
		const int32 Index010 = Indices[(MinX + 0) + SizeX * (MinY + 1) + SizeXY * (MinZ + 0)];
		IGNORE_PERF_WARNING
		const int32 Index011 = Indices[(MinX + 1) + SizeX * (MinY + 1) + SizeXY * (MinZ + 0)];
		IGNORE_PERF_WARNING
		const int32 Index100 = Indices[(MinX + 0) + SizeX * (MinY + 0) + SizeXY * (MinZ + 1)];
		IGNORE_PERF_WARNING
		const int32 Index101 = Indices[(MinX + 1) + SizeX * (MinY + 0) + SizeXY * (MinZ + 1)];
		IGNORE_PERF_WARNING
		const int32 Index110 = Indices[(MinX + 0) + SizeX * (MinY + 1) + SizeXY * (MinZ + 1)];
		IGNORE_PERF_WARNING
		const int32 Index111 = Indices[(MinX + 1) + SizeX * (MinY + 1) + SizeXY * (MinZ + 1)];

		{
			float OldValueR;
			if (Query.Indirection)
			{
				IGNORE_PERF_WARNING
				OldValueR = InOutIndirectValuesR[Query.Indirection[Index]];
			}
			else
			{
				IGNORE_PERF_WARNING
				OldValueR = InOutIndirectValuesR[Index];
			}

			const float NewValueR = TrilinearInterpolation(
				IGNORE_PERF_WARNING
				MeshValuesR[Index000],
				IGNORE_PERF_WARNING
				MeshValuesR[Index001],
				IGNORE_PERF_WARNING
				MeshValuesR[Index010],
				IGNORE_PERF_WARNING
				MeshValuesR[Index011],
				IGNORE_PERF_WARNING
				MeshValuesR[Index100],
				IGNORE_PERF_WARNING
				MeshValuesR[Index101],
				IGNORE_PERF_WARNING
				MeshValuesR[Index110],
				IGNORE_PERF_WARNING
				MeshValuesR[Index111],
				AlphaX,
				AlphaY,
				AlphaZ);

			if (Query.Indirection)
			{
				IGNORE_PERF_WARNING
				InOutIndirectValuesR[Query.Indirection[Index]] = lerp(OldValueR, NewValueR, Alpha);
			}
			else
			{
				IGNORE_PERF_WARNING
				InOutIndirectValuesR[Index] = lerp(OldValueR, NewValueR, Alpha);
			}
		}

		{
			float OldValueG;
			if (Query.Indirection)
			{
				IGNORE_PERF_WARNING
				OldValueG = InOutIndirectValuesG[Query.Indirection[Index]];
			}
			else
			{
				IGNORE_PERF_WARNING
				OldValueG = InOutIndirectValuesG[Index];
			}

			const float NewValueG = TrilinearInterpolation(
				IGNORE_PERF_WARNING
				MeshValuesG[Index000],
				IGNORE_PERF_WARNING
				MeshValuesG[Index001],
				IGNORE_PERF_WARNING
				MeshValuesG[Index010],
				IGNORE_PERF_WARNING
				MeshValuesG[Index011],
				IGNORE_PERF_WARNING
				MeshValuesG[Index100],
				IGNORE_PERF_WARNING
				MeshValuesG[Index101],
				IGNORE_PERF_WARNING
				MeshValuesG[Index110],
				IGNORE_PERF_WARNING
				MeshValuesG[Index111],
				AlphaX,
				AlphaY,
				AlphaZ);

			if (Query.Indirection)
			{
				IGNORE_PERF_WARNING
				InOutIndirectValuesG[Query.Indirection[Index]] = lerp(OldValueG, NewValueG, Alpha);
			}
			else
			{
				IGNORE_PERF_WARNING
				InOutIndirectValuesG[Index] = lerp(OldValueG, NewValueG, Alpha);
			}
		}

		{
			float OldValueB;
			if (Query.Indirection)
			{
				IGNORE_PERF_WARNING
				OldValueB = InOutIndirectValuesB[Query.Indirection[Index]];
			}
			else
			{
				IGNORE_PERF_WARNING
				OldValueB = InOutIndirectValuesB[Index];
			}

			const float NewValueB = TrilinearInterpolation(
				IGNORE_PERF_WARNING
				MeshValuesB[Index000],
				IGNORE_PERF_WARNING
				MeshValuesB[Index001],
				IGNORE_PERF_WARNING
				MeshValuesB[Index010],
				IGNORE_PERF_WARNING
				MeshValuesB[Index011],
				IGNORE_PERF_WARNING
				MeshValuesB[Index100],
				IGNORE_PERF_WARNING
				MeshValuesB[Index101],
				IGNORE_PERF_WARNING
				MeshValuesB[Index110],
				IGNORE_PERF_WARNING
				MeshValuesB[Index111],
				AlphaX,
				AlphaY,
				AlphaZ);

			if (Query.Indirection)
			{
				IGNORE_PERF_WARNING
				InOutIndirectValuesB[Query.Indirection[Index]] = lerp(OldValueB, NewValueB, Alpha);
			}
			else
			{
				IGNORE_PERF_WARNING
				InOutIndirectValuesB[Index] = lerp(OldValueB, NewValueB, Alpha);
			}
		}

		{
			float OldValueA;
			if (Query.Indirection)
			{
				IGNORE_PERF_WARNING
				OldValueA = InOutIndirectValuesA[Query.Indirection[Index]];
			}
			else
			{
				IGNORE_PERF_WARNING
				OldValueA = InOutIndirectValuesA[Index];
			}

			const float NewValueA = TrilinearInterpolation(
				IGNORE_PERF_WARNING
				MeshValuesA[Index000],
				IGNORE_PERF_WARNING
				MeshValuesA[Index001],
				IGNORE_PERF_WARNING
				MeshValuesA[Index010],
				IGNORE_PERF_WARNING
				MeshValuesA[Index011],
				IGNORE_PERF_WARNING
				MeshValuesA[Index100],
				IGNORE_PERF_WARNING
				MeshValuesA[Index101],
				IGNORE_PERF_WARNING
				MeshValuesA[Index110],
				IGNORE_PERF_WARNING
				MeshValuesA[Index111],
				AlphaX,
				AlphaY,
				AlphaZ);

			if (Query.Indirection)
			{
				IGNORE_PERF_WARNING
				InOutIndirectValuesA[Query.Indirection[Index]] = lerp(OldValueA, NewValueA, Alpha);
			}
			else
			{
				IGNORE_PERF_WARNING
				InOutIndirectValuesA[Index] = lerp(OldValueA, NewValueA, Alpha);
			}
		}
	}
}

export void VoxelMeshStamp_ApplyMetadata_Normal(
	const uniform FVoxelVolumeSparseQuery& Query,
	const uniform FVoxelVolumeTransform& StampToQuery,
	uniform FVoxelOctahedron InOutIndirectValues[],
	const uniform float Alphas[],
	const uniform int3& Size,
	const uniform float VoxelSize,
	const uniform float3& MeshOrigin,
	const uniform int32 Indices[],
	const uniform FVoxelOctahedron MeshValues[])
{
	const uniform float MaxPositionX = Size.x - 2.0001;
	const uniform float MaxPositionY = Size.y - 2.0001;
	const uniform float MaxPositionZ = Size.z - 2.0001;

	const uniform float InvVoxelSize = 1.f / VoxelSize;

	const uniform int32 SizeX = Size.x;
	const uniform int32 SizeXY = Size.x * Size.y;

	FOREACH(Index, 0, Query.Num)
	{
		const varying float Alpha = Alphas[Index];
		coherent_if (Alpha == 0.f)
		{
			continue;
		}

		const varying double3 WorldPosition = MakeDouble3(
			Query.PositionsX[Index],
			Query.PositionsY[Index],
			Query.PositionsZ[Index]);

		varying float3 LocalPosition = InverseTransform(StampToQuery, WorldPosition);
		LocalPosition = LocalPosition * InvVoxelSize - MeshOrigin;

		LocalPosition.x = clamp(LocalPosition.x, 1.f, MaxPositionX);
		LocalPosition.y = clamp(LocalPosition.y, 1.f, MaxPositionY);
		LocalPosition.z = clamp(LocalPosition.z, 1.f, MaxPositionZ);

		const varying float MinXf = floor(LocalPosition.x);
		const varying float MinYf = floor(LocalPosition.y);
		const varying float MinZf = floor(LocalPosition.z);

		const varying float AlphaX = LocalPosition.x - MinXf;
		const varying float AlphaY = LocalPosition.y - MinYf;
		const varying float AlphaZ = LocalPosition.z - MinZf;

		const varying int32 MinX = (int32)MinXf;
		const varying int32 MinY = (int32)MinYf;
		const varying int32 MinZ = (int32)MinZf;

		check(0 <= MinX && MinX + 1 < Size.x);
		check(0 <= MinY && MinY + 1 < Size.y);
		check(0 <= MinZ && MinZ + 1 < Size.z);

		IGNORE_PERF_WARNING
		const int32 Index000 = Indices[(MinX + 0) + SizeX * (MinY + 0) + SizeXY * (MinZ + 0)];
		IGNORE_PERF_WARNING
		const int32 Index001 = Indices[(MinX + 1) + SizeX * (MinY + 0) + SizeXY * (MinZ + 0)];
		IGNORE_PERF_WARNING
		const int32 Index010 = Indices[(MinX + 0) + SizeX * (MinY + 1) + SizeXY * (MinZ + 0)];
		IGNORE_PERF_WARNING
		const int32 Index011 = Indices[(MinX + 1) + SizeX * (MinY + 1) + SizeXY * (MinZ + 0)];
		IGNORE_PERF_WARNING
		const int32 Index100 = Indices[(MinX + 0) + SizeX * (MinY + 0) + SizeXY * (MinZ + 1)];
		IGNORE_PERF_WARNING
		const int32 Index101 = Indices[(MinX + 1) + SizeX * (MinY + 0) + SizeXY * (MinZ + 1)];
		IGNORE_PERF_WARNING
		const int32 Index110 = Indices[(MinX + 0) + SizeX * (MinY + 1) + SizeXY * (MinZ + 1)];
		IGNORE_PERF_WARNING
		const int32 Index111 = Indices[(MinX + 1) + SizeX * (MinY + 1) + SizeXY * (MinZ + 1)];

		float3 OldValue;
		if (Query.Indirection)
		{
			IGNORE_PERF_WARNING
			OldValue = OctahedronToUnitVector(InOutIndirectValues[Query.Indirection[Index]]);
		}
		else
		{
			IGNORE_PERF_WARNING
			OldValue = OctahedronToUnitVector(InOutIndirectValues[Index]);
		}

		const float3 NewValue = normalize(TrilinearInterpolation(
			IGNORE_PERF_WARNING
			OctahedronToUnitVector(MeshValues[Index000]),
			IGNORE_PERF_WARNING
			OctahedronToUnitVector(MeshValues[Index001]),
			IGNORE_PERF_WARNING
			OctahedronToUnitVector(MeshValues[Index010]),
			IGNORE_PERF_WARNING
			OctahedronToUnitVector(MeshValues[Index011]),
			IGNORE_PERF_WARNING
			OctahedronToUnitVector(MeshValues[Index100]),
			IGNORE_PERF_WARNING
			OctahedronToUnitVector(MeshValues[Index101]),
			IGNORE_PERF_WARNING
			OctahedronToUnitVector(MeshValues[Index110]),
			IGNORE_PERF_WARNING
			OctahedronToUnitVector(MeshValues[Index111]),
			AlphaX,
			AlphaY,
			AlphaZ));

		if (Query.Indirection)
		{
			IGNORE_PERF_WARNING
			InOutIndirectValues[Query.Indirection[Index]] = MakeOctahedron(UnitVectorToOctahedron(lerp(OldValue, NewValue, Alpha)));
		}
		else
		{
			IGNORE_PERF_WARNING
			InOutIndirectValues[Index] = MakeOctahedron(UnitVectorToOctahedron(lerp(OldValue, NewValue, Alpha)));
		}
	}
}