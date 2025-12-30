// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "MegaMaterial/VoxelMaterialSubsystem.h"
#include "MegaMaterial/VoxelMegaMaterialProxy.h"

#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"

TSharedRef<FVoxelMaterialInstanceRef> FVoxelMaterialSubsystem::GetMaterialInstanceRef(const EVoxelMegaMaterialTarget Target) const
{
	return MaterialInstanceRefs[int32(Target)].ToSharedRef();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMaterialSubsystem::LoadFromPrevious(FVoxelSubsystem& InPreviousSubsystem)
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelMaterialSubsystem& PreviousSubsystem = CastStructChecked<FVoxelMaterialSubsystem>(InPreviousSubsystem);

	if (PreviousSubsystem.GetConfig().MegaMaterialProxy == GetConfig().MegaMaterialProxy)
	{
		MaterialInstanceRefs = PreviousSubsystem.MaterialInstanceRefs;
	}
}

void FVoxelMaterialSubsystem::Initialize()
{
	VOXEL_FUNCTION_COUNTER();

	for (const EVoxelMegaMaterialTarget Target : TEnumRange<EVoxelMegaMaterialTarget>())
	{
		UMaterialInterface* Material = GetConfig().MegaMaterialProxy->GetTargetMaterial(Target).Resolve();

		TSharedPtr<FVoxelMaterialInstanceRef>& MaterialInstanceRef = MaterialInstanceRefs[int32(Target)];
		if (MaterialInstanceRef)
		{
			if (const UMaterialInstanceDynamic* Instance = MaterialInstanceRef->GetInstance())
			{
				if (Instance->Parent != Material)
				{
					MaterialInstanceRef.Reset();
				}
			}
		}

		if (MaterialInstanceRef)
		{
			continue;
		}

		UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(Material, GetTransientPackage());
		check(Instance);
		MaterialInstanceRef = FVoxelMaterialInstanceRef::Make(*Instance);
	}
}

void FVoxelMaterialSubsystem::Render(FVoxelRuntime& Runtime)
{
	VOXEL_FUNCTION_COUNTER();

	for (const TSharedPtr<FVoxelMaterialInstanceRef>& MaterialInstanceRef : MaterialInstanceRefs)
	{
		UMaterialInstanceDynamic* Instance = MaterialInstanceRef->GetInstance();
		if (!ensure(Instance))
		{
			continue;
		}

		GetConfig().MegaMaterialProxy->UpdateInstance(*Instance);
	}
}