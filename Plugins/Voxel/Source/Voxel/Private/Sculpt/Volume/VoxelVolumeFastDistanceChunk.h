// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelVolumeBlendMode.h"
#include "Sculpt/Volume/VoxelVolumeSculptDefinitions.h"

DECLARE_VOXEL_MEMORY_STAT(VOXEL_API, STAT_VoxelVolumeFastDistance_Memory, "Voxel Volume Fast Distance Memory");

struct FVoxelVolumeFastDistanceChunk
	: FVoxelVolumeSculptDefinitions
	, TVoxelRefCountThis<FVoxelVolumeFastDistanceChunk>
{
public:
	static constexpr float DistanceScale = 0.1087f; // ~ (UE_SQRT_3 * ChunkSize) / 127.5
	static constexpr float DistanceOffset = 0.5f; // Avoid 0

	FORCEINLINE static float Unpack(const int8 Value)
	{
		return (Value + DistanceOffset) * DistanceScale;
	}
	FORCEINLINE static int8 Pack(const float Value, const bool bCheckBounds)
	{
		const int32 RoundedValue = FMath::RoundToInt(Value / DistanceScale - DistanceOffset);
		if (bCheckBounds)
		{
			ensureVoxelSlow(-128 <= RoundedValue && RoundedValue <= 127);
		}
		return FMath::Clamp(RoundedValue, MIN_int8, MAX_int8);
	}

public:
	TVoxelStaticArray<int8, ChunkCount> Distances{ NoInit };

	FVoxelVolumeFastDistanceChunk();
	UE_NONCOPYABLE(FVoxelVolumeFastDistanceChunk)

	VOXEL_COUNT_INSTANCES();
	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelVolumeFastDistance_Memory);

	FORCEINLINE float GetDistance(const int32 Index) const
	{
		return Unpack(Distances[Index]);
	}

public:
	void Serialize(
		FArchive& Ar,
		int32 Version);

	int64 GetAllocatedSize() const;
	TVoxelRefCountPtr<FVoxelVolumeFastDistanceChunk> Clone() const;

public:
	static FORCEINLINE float Blend(
		const float OldDistance,
		const float NewDistance,
		const EVoxelVolumeBlendMode BlendMode,
		const bool bApplyOnVoid)
	{
		switch (BlendMode)
		{
		default: VOXEL_ASSUME(false);
		case EVoxelVolumeBlendMode::Additive:
		{
			if (FVoxelUtilities::IsNaN(NewDistance))
			{
				return OldDistance;
			}
			if (FVoxelUtilities::IsNaN(OldDistance))
			{
				if (bApplyOnVoid)
				{
					return NewDistance;
				}
				else
				{
					return OldDistance;
				}
			}

			return FMath::Min(OldDistance, NewDistance);
		}
		case EVoxelVolumeBlendMode::Subtractive:
		{
			if (FVoxelUtilities::IsNaN(NewDistance))
			{
				return OldDistance;
			}
			if (FVoxelUtilities::IsNaN(OldDistance))
			{
				if (bApplyOnVoid)
				{
					return NewDistance;
				}
				else
				{
					return OldDistance;
				}
			}

			return FMath::Max(OldDistance, NewDistance);
		}
		case EVoxelVolumeBlendMode::Intersect:
		{
			if (FVoxelUtilities::IsNaN(OldDistance))
			{
				return OldDistance;
			}
			if (FVoxelUtilities::IsNaN(NewDistance))
			{
				return NewDistance;
			}

			return FMath::Max(OldDistance, NewDistance);
		}
		case EVoxelVolumeBlendMode::Override:
		{
			return NewDistance;
		}
		}
	}
};