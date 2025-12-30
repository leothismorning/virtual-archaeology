// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Surface/VoxelSurfaceTypeInterface.h"
#include "Surface/VoxelSurfaceTypeAsset.h"
#include "Surface/VoxelSurfaceTypeTable.h"

#if WITH_EDITOR
#include "FileHelpers.h"
#endif
#include "Materials/MaterialInterface.h"

void UVoxelSurfaceTypeInterface::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	SerializeVoxelVersion(Ar);
}

#if WITH_EDITOR
void UVoxelSurfaceTypeInterface::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		FVoxelSurfaceTypeTable::Refresh();
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
UVoxelSurfaceTypeAsset* UVoxelSurfaceTypeInterface::Migrate(UObject* Material)
{
	VOXEL_FUNCTION_COUNTER();

	if (!Material)
	{
		return nullptr;
	}

	// Legacy migration
	if (UVoxelSurfaceTypeAsset* Asset = LoadObject<UVoxelSurfaceTypeAsset>(
		nullptr,
		*FString("/Game/VM_" + Material->GetName()),
		nullptr,
		LOAD_NoWarn))
	{
		if (Asset->LegacyMaterial == Material)
		{
			return Asset;
		}
	}

	// Legacy migration
	if (UVoxelSurfaceTypeAsset* Asset = LoadObject<UVoxelSurfaceTypeAsset>(
		nullptr,
		*FString("/Game/VS_" + Material->GetName()),
		nullptr,
		LOAD_NoWarn))
	{
		if (Asset->LegacyMaterial == Material)
		{
			return Asset;
		}
	}

	FString Path = "/Game/VST_" + Material->GetName();

	while (true)
	{
		UVoxelSurfaceTypeAsset* Asset = LoadObject<UVoxelSurfaceTypeAsset>(
			nullptr,
			*Path,
			nullptr,
			LOAD_NoWarn);

		if (!Asset)
		{
			break;
		}

		if (Asset->LegacyMaterial == Material)
		{
			return Asset;
		}

		Path += "_New";
	}

	UVoxelSurfaceTypeAsset* Asset = FVoxelUtilities::CreateNewAsset_Direct<UVoxelSurfaceTypeAsset>(Path, {});
	if (!ensure(Asset))
	{
		return nullptr;
	}

	FVoxelUtilities::DelayedCall([WeakAsset = MakeVoxelObjectPtr(Asset)]
	{
		const UVoxelSurfaceTypeAsset* LocalAsset = WeakAsset.Resolve();
		if (!ensure(LocalAsset))
		{
			return;
		}

		UEditorLoadingAndSavingUtils::SavePackages(
			TArray<UPackage*>
			{
				LocalAsset->GetOutermost()
			},
			false);
	});

	Asset->Material = Cast<UMaterialInterface>(Material);
	Asset->LegacyMaterial = Material;
	return Asset;
}

void UVoxelSurfaceTypeInterface::Migrate(
	TObjectPtr<UObject>& Material,
	TObjectPtr<UVoxelSurfaceTypeInterface>& SurfaceType)
{
	if (!Material ||
		!ensure(!SurfaceType))
	{
		return;
	}

	SurfaceType = Migrate(Material);
	Material = {};
}

void UVoxelSurfaceTypeInterface::Migrate(
	TArray<TObjectPtr<UMaterialInterface>>& Materials,
	TArray<TObjectPtr<UVoxelSurfaceTypeAsset>>& SurfaceTypes)
{
	if (Materials.Num() == 0)
	{
		return;
	}

	if (!ensure(SurfaceTypes.Num() == 0))
	{
		return;
	}

	VOXEL_FUNCTION_COUNTER();

	for (UMaterialInterface* OldMaterial :  Materials)
	{
		SurfaceTypes.Add(Migrate(OldMaterial));
	}

	check(SurfaceTypes.Num() == Materials.Num());
	Materials.Reset();
}
#endif