// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelInvalidationCallstack.h"
#include "VoxelPCGTracker.generated.h"

class UPCGComponent;
class UPCGSettings;

USTRUCT()
struct VOXELPCG_API FVoxelInvalidationFrame_PCG : public FVoxelInvalidationFrame
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	TVoxelObjectPtr<const UPCGComponent> Component;
	TVoxelObjectPtr<const UPCGSettings> Settings;
	uint64 SettingsId = 0;

	static TSharedRef<FVoxelInvalidationCallstack> Create(
		TVoxelObjectPtr<const UPCGComponent> Component,
		const UPCGSettings& Settings);

	//~ Begin FVoxelInvalidationSource Interface
	virtual uint64 GetHash() const override;
	virtual FString ToString() const override;
	//~ End FVoxelInvalidationSource Interface
};

class VOXELPCG_API FVoxelPCGTracker : public FVoxelSingleton
{
public:
	void RegisterDependencyCollector(
		FVoxelDependencyCollector& DependencyCollector,
		const FVoxelInvalidationQueue& InvalidationQueue,
		UPCGComponent& Component,
		uint64 SettingsId,
		const FSharedVoidPtr& PtrToKeepAlive = nullptr);

	uint32 GetCRC(
		UPCGComponent& Component,
		uint64 SettingsId);

#if VOXEL_INVALIDATION_TRACKING
	TVoxelArray<TSharedRef<const FVoxelInvalidationCallstack>> GetCallstacks(
		UPCGComponent& Component,
		uint64 SettingsId);
#endif

	//~ Begin FVoxelSingleton Interface
	virtual void Tick() override;
	//~ End FVoxelSingleton Interface

private:
	struct FData
	{
		uint32 CRC = AllocateCRC();
		TSharedPtr<FVoxelDependencyTracker> DependencyTracker;
		FSharedVoidPtr PtrToKeepAlive;
	};
	struct FKey
	{
		TVoxelObjectPtr<UPCGComponent> Component;
		uint64 SettingsId = 0;

		FORCEINLINE bool operator==(const FKey& Other) const
		{
			return
				Component == Other.Component &&
				SettingsId == Other.SettingsId;
		}
		FORCEINLINE friend uint32 GetTypeHash(const FKey& Key)
		{
			return HashCombine(
				GetTypeHash(Key.Component),
				GetTypeHash(Key.SettingsId));
		}
	};

	double LastCleanup = 0;
	TVoxelSet<FKey> KeysToRefresh;
	TVoxelMap<FKey, FData> KeyToData;
#if VOXEL_INVALIDATION_TRACKING
	TVoxelMap<FKey, TVoxelArray<TSharedRef<const FVoxelInvalidationCallstack>>> KeyToCallstacks;
#endif

	static uint32 AllocateCRC();
	static void RefreshComponent(UPCGComponent& Component);
};

extern VOXELPCG_API bool GVoxelDisablePCGRegen;
extern VOXELPCG_API FVoxelPCGTracker* GVoxelPCGTracker;