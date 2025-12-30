// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMetadata.h"
#include "VoxelFloatMetadata.h"
#if WITH_EDITOR
#include "ObjectTools.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/AssetRegistryModule.h"
#endif

FVoxelPinValue UVoxelMetadata::GetDefaultValue() const
{
	return FVoxelPinValue(GetInnerType());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
bool UVoxelMetadata::CanMigrate()
{
	const IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	return !AssetRegistry.IsLoadingAssets();
}

UVoxelMetadata* UVoxelMetadata::Migrate(const FName LegacyName)
{
	VOXEL_FUNCTION_COUNTER();

	if (LegacyName.IsNone())
	{
		return nullptr;
	}

	UVoxelFloatMetadata* Metadata = nullptr;
	ForEachAssetOfClass<UVoxelFloatMetadata>([&](UVoxelFloatMetadata& OtherMetadata)
	{
		if (OtherMetadata.LegacyName == LegacyName)
		{
			ensure(!Metadata);
			Metadata = &OtherMetadata;
		}
	});

	if (Metadata)
	{
		return Metadata;
	}

	Metadata = FVoxelUtilities::CreateNewAsset_Direct<UVoxelFloatMetadata>("/Game/VoxelFloatMetadata_" + LegacyName.ToString(), {});
	if (!ensure(Metadata))
	{
		return nullptr;
	}

	Metadata->LegacyName = LegacyName;
	return Metadata;
}

void UVoxelMetadata::Migrate(
	TArray<FName>& LegacyNames,
	TArray<TObjectPtr<UVoxelMetadata>>& NewMetadatas)
{
	VOXEL_FUNCTION_COUNTER();

	if (!CanMigrate())
	{
		return;
	}

	for (const FName Name : LegacyNames)
	{
		NewMetadatas.Add(Migrate(Name));
	}
	LegacyNames.Empty();
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelMetadata::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	SerializeVoxelVersion(Ar);
}

#if WITH_EDITOR
void UVoxelMetadata::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!AssetIcon.bCustomIcon)
	{
		ThumbnailTools::CacheEmptyThumbnail(GetFullName(), GetPackage());
	}

	FVoxelMetadataRef(this).UpdateFromSourceObject();
}

void UVoxelMetadata::PostRename(UObject* OldOuter, const FName OldName)
{
	Super::PostRename(OldOuter, OldName);
	FVoxelMetadataRef(this).UpdateFromSourceObject();
}
#endif