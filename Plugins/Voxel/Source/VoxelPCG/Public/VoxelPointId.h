// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Buffer/VoxelBaseBuffers.h"
#include "VoxelPointId.generated.h"

USTRUCT()
struct VOXELPCG_API FVoxelPointId
{
	GENERATED_BODY()

	UPROPERTY()
	uint64 PointId = 0;

	FVoxelPointId() = default;
	FVoxelPointId(const uint64 PointId)
		: PointId(PointId)
	{
	}

	FORCEINLINE bool IsValid() const
	{
		return PointId != 0;
	}

	FORCEINLINE bool operator==(const FVoxelPointId& Other) const
	{
		return PointId == Other.PointId;
	}
	FORCEINLINE friend uint32 GetTypeHash(const FVoxelPointId& Id)
	{
		return GetTypeHash(Id.PointId);
	}
};
checkStatic(sizeof(FVoxelPointId) == sizeof(uint64));

struct VOXELPCG_API FVoxelPointRandom
{
	const uint64 BaseHash;

	FVoxelPointRandom(
		const FVoxelSeed Seed,
		const uint64 UniqueId)
		: BaseHash(FVoxelUtilities::MurmurHashMulti(Seed, UniqueId))
	{
	}

	FORCEINLINE float GetFraction(const FVoxelPointId PointId) const
	{
		return FVoxelUtilities::GetFraction(BaseHash ^ PointId.PointId);
	}
	FORCEINLINE FVoxelPointId MakeId(const FVoxelPointId PointId, const int32 Index) const
	{
		return FVoxelUtilities::MurmurHash(BaseHash ^ FVoxelUtilities::MurmurHash(PointId, Index));
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DECLARE_VOXEL_TERMINAL_BUFFER(FVoxelPointIdBuffer, FVoxelPointId);

USTRUCT(DisplayName = "Point Id Buffer")
struct VOXELPCG_API FVoxelPointIdBuffer final : public FVoxelTerminalBuffer
{
	GENERATED_BODY()
	GENERATED_VOXEL_TERMINAL_BUFFER_BODY(FVoxelPointIdBuffer, FVoxelPointId);
};