// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelPCGTracker.h"
#include "PCGGraph.h"
#include "PCGComponent.h"
#include "PCGSubsystem.h"

#if WITH_EDITOR
#include "Misc/ConfigCacheIni.h"
#endif

VOXEL_CONSOLE_VARIABLE(
	VOXELPCG_API, bool, GVoxelDisablePCGRegen, false,
	"voxel.pcg.DisableRegen",
	"Disable PCG refreshes triggered by voxel changes");

FVoxelPCGTracker* GVoxelPCGTracker = new FVoxelPCGTracker();

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

TSharedRef<FVoxelInvalidationCallstack> FVoxelInvalidationFrame_PCG::Create(
	const TVoxelObjectPtr<const UPCGComponent> Component,
	const UPCGSettings& Settings)
{
	FVoxelInvalidationFrame_PCG Frame;
	Frame.Component = Component;
	Frame.Settings = Settings;
	Frame.SettingsId = Settings.GetStableUID();
	return FVoxelInvalidationCallstack::Create(Frame);
}

uint64 FVoxelInvalidationFrame_PCG::GetHash() const
{
	return FVoxelUtilities::MurmurHashMulti(Component, Settings, SettingsId);
}

FString FVoxelInvalidationFrame_PCG::ToString() const
{
	FString Result = Component.GetReadableName();
	if (const UPCGSettings* SettingsObject = Settings.Resolve())
	{
#if WITH_EDITOR
		Result += " Graph: " + MakeVoxelObjectPtr(SettingsObject->GetTypedOuter<UPCGGraph>()).GetReadableName();
		Result += " Node:" + SettingsObject->GetDefaultNodeTitle().ToString();
#endif
	}
	return Result;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void FVoxelPCGTracker::RegisterDependencyCollector(
	FVoxelDependencyCollector& DependencyCollector,
	const FVoxelInvalidationQueue& InvalidationQueue,
	UPCGComponent& Component,
	const uint64 SettingsId,
	const FSharedVoidPtr& PtrToKeepAlive)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	const FKey Key
	{
		&Component,
		SettingsId
	};

	const TSharedRef<FVoxelDependencyTracker> DependencyTracker = DependencyCollector.Finalize(
		&InvalidationQueue,
		[this, Key](const FVoxelInvalidationCallstack& Callstack)
		{
#if VOXEL_INVALIDATION_TRACKING
			const TSharedRef<const FVoxelInvalidationCallstack> SharedCallstack = Callstack.AsShared();
#endif

			Voxel::GameTask([=, this]
			{
				KeysToRefresh.Add(Key);
#if VOXEL_INVALIDATION_TRACKING
				KeyToCallstacks.FindOrAdd(Key).Add(SharedCallstack);
#endif
			});
		});

	FData& Data = KeyToData.FindOrAdd(Key);
	Data.DependencyTracker = DependencyTracker;
	Data.PtrToKeepAlive = PtrToKeepAlive;
}

uint32 FVoxelPCGTracker::GetCRC(
	UPCGComponent& Component,
	const uint64 SettingsId)
{
	check(IsInGameThread());

	const FKey Key
	{
		&Component,
		SettingsId
	};

	return KeyToData.FindOrAdd(Key).CRC;
}

#if VOXEL_INVALIDATION_TRACKING
TVoxelArray<TSharedRef<const FVoxelInvalidationCallstack>> FVoxelPCGTracker::GetCallstacks(
	UPCGComponent& Component,
	const uint64 SettingsId)
{
	TVoxelArray<TSharedRef<const FVoxelInvalidationCallstack>> Callstacks;

	KeyToCallstacks.RemoveAndCopyValue(
		FKey
		{
			&Component,
			SettingsId
		},
		Callstacks);

	return Callstacks;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void FVoxelPCGTracker::Tick()
{
	VOXEL_FUNCTION_COUNTER();

	{
		TVoxelArray<FKey> LocalKeysToRefresh;
		for (auto It = KeysToRefresh.CreateIterator(); It; ++It)
		{
			const UPCGComponent* Component = It->Component.Resolve();
			if (!Component)
			{
				It.RemoveCurrent();
				continue;
			}

			if (Component->IsGenerating())
			{
				// Can't refresh yet
				LOG_VOXEL(Verbose, "Cannot refresh %s: still generating", *Component->GetPathName());
				continue;
			}

			LocalKeysToRefresh.Add(*It);
			It.RemoveCurrent();
		}

		for (const FKey& Key : LocalKeysToRefresh)
		{
			KeyToData.FindOrAdd(Key).CRC = AllocateCRC();

			if (UPCGComponent* Component = Key.Component.Resolve_Ensured())
			{
				RefreshComponent(*Component);
			}
		}
	}

	if (FPlatformTime::Seconds() - LastCleanup < 30)
	{
		return;
	}
	LastCleanup = FPlatformTime::Seconds();

	for (auto It = KeyToData.CreateIterator(); It; ++It)
	{
		if (!It.Key().Component.IsValid_Slow())
		{
			It.RemoveCurrent();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

uint32 FVoxelPCGTracker::AllocateCRC()
{
	static FVoxelCounter64 Counter;
	return uint32(Counter.Increment_ReturnNew());
}

void FVoxelPCGTracker::RefreshComponent(UPCGComponent& Component)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	ensure(!Component.IsGenerating());

	if (GVoxelDisablePCGRegen)
	{
		return;
	}

	LOG_VOXEL(Verbose, "Refreshing %s", *Component.GetPathName());

#if WITH_EDITOR
	if (GIsEditor)
	{
		if (const UWorld* World = Component.GetWorld())
		{
			if (!World->IsGameWorld())
			{
				Component.DirtyGenerated();
				Component.Refresh();
				return;
			}
		}
	}
#endif

	if (!Component.IsManagedByRuntimeGenSystem())
	{
		return;
	}

	UPCGSubsystem* Subsystem = Component.GetSubsystem();
	if (!ensure(Subsystem))
	{
		return;
	}

	Subsystem->RefreshRuntimeGenComponent(&Component, EPCGChangeType::None);
}