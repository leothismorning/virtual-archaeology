// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "MegaMaterial/VoxelMegaMaterialUsageTracker.h"
#include "MegaMaterial/VoxelMegaMaterial.h"
#include "MegaMaterial/VoxelMegaMaterialGeneratedData.h"
#include "Surface/VoxelSurfaceTypeAsset.h"

#if WITH_EDITOR
void FVoxelMegaMaterialUsageTracker::Sync()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	UVoxelMegaMaterial* MegaMaterial = WeakMegaMaterial.Resolve();
	if (!ensureVoxelSlow(MegaMaterial))
	{
		return;
	}

	if (!MegaMaterial->bDetectNewSurfaces)
	{
		VOXEL_SCOPE_LOCK(CriticalSection);
		SurfaceTypesToAdd_RequiresLock.Empty();
		return;
	}

	TVoxelArray<FVoxelSurfaceType> NewSurfaceTypes;
	{
		VOXEL_SCOPE_LOCK(CriticalSection);

		NewSurfaceTypes = SurfaceTypesToAdd_RequiresLock.Array();
		SurfaceTypesToAdd_RequiresLock.Reset();
	}

	for (const FVoxelSurfaceType& SurfaceType : NewSurfaceTypes)
	{
		UVoxelSurfaceTypeAsset* SurfaceTypeAsset = SurfaceType.GetSurfaceTypeAsset().Resolve();
		if (!ensureVoxelSlow(SurfaceTypeAsset) ||
			MegaMaterial->SurfaceTypes.Contains(SurfaceTypeAsset))
		{
			continue;
		}

		if (MegaMaterial->bEditorOnly)
		{
			MegaMaterial->Modify();
			MegaMaterial->SurfaceTypes.Remove(nullptr);
			MegaMaterial->SurfaceTypes.AddUnique(SurfaceTypeAsset);
			MegaMaterial->GetGeneratedData_EditorOnly().QueueRebuild();
			continue;
		}

		if (const TSharedPtr<FVoxelNotification> Notification = SurfaceTypeToWeakNotification.FindRef(SurfaceTypeAsset).Pin())
		{
			Notification->ResetExpiration();
			continue;
		}

		const TSharedRef<FVoxelNotification> Notification = FVoxelNotification::Create("Missing MegaMaterial entry");

		Notification->SetSubText(
			"Add " + SurfaceTypeAsset->GetName() + " to " + MegaMaterial->GetName() + "?\n\n" +
			"This is required to render this surface on a Voxel World");

		Notification->AddButton_ExpireOnClick(
			"Add",
			"Add this surface to the MegaMaterial array. This will rebuild the MegaMaterial.",
			MakeWeakObjectPtrLambda(MegaMaterial, MakeWeakPtrLambda(this, [this, MegaMaterial, WeakSurfaceTypeAsset = MakeVoxelObjectPtr(SurfaceTypeAsset)]
			{
				UVoxelSurfaceTypeAsset* LocalSurfaceTypeAsset = WeakSurfaceTypeAsset.Resolve();
				if (!ensure(LocalSurfaceTypeAsset))
				{
					return;
				}

				MegaMaterial->Modify();
				MegaMaterial->SurfaceTypes.Remove(nullptr);
				MegaMaterial->SurfaceTypes.AddUnique(LocalSurfaceTypeAsset);
				MegaMaterial->GetGeneratedData_EditorOnly().QueueRebuild();
			})));

		Notification->ExpireAndFadeoutIn(10.f);

		SurfaceTypeToWeakNotification.FindOrAdd(SurfaceTypeAsset) = Notification;
	}
}

void FVoxelMegaMaterialUsageTracker::NotifyMissingSurfaceType(const FVoxelSurfaceType SurfaceType)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(!SurfaceType.IsNull()))
	{
		return;
	}

	VOXEL_SCOPE_LOCK(CriticalSection);

	if (SurfaceTypesToAdd_RequiresLock.Contains(SurfaceType))
	{
		return;
	}

	SurfaceTypesToAdd_RequiresLock.Add_EnsureNew(SurfaceType);

	Voxel::GameTask_Async(MakeWeakPtrLambda(this, [this]
	{
		Sync();
	}));
}
#endif