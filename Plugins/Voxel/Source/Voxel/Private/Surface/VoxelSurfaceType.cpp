// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Surface/VoxelSurfaceType.h"
#include "Surface/VoxelSurfaceTypeTable.h"
#include "Surface/VoxelSurfaceTypeAsset.h"
#include "Surface/VoxelSmartSurfaceType.h"
#include "VoxelInvalidationCallstack.h"

#if !UE_BUILD_SHIPPING
TVoxelArray<FVoxelObjectPtr> GVoxelDebugSurfaceTypes;
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class FVoxelSurfaceTypeManager : public FVoxelSingleton
{
public:
	FVoxelSharedCriticalSection CriticalSection;

	TVoxelArray<TVoxelObjectPtr<UVoxelSurfaceTypeAsset>> SurfaceTypeAssets_RequiresLock;
	TVoxelArray<TVoxelObjectPtr<UVoxelSmartSurfaceType>> SmartSurfaceTypes_RequiresLock;

	TVoxelMap<TObjectPtr<UVoxelSurfaceTypeAsset>, uint16> SurfaceTypeAssetToIndex_RequiresLock;
	TVoxelMap<TObjectPtr<UVoxelSmartSurfaceType>, uint16> SmartSurfaceTypeToIndex_RequiresLock;

	TVoxelFixedBitArray<FVoxelSurfaceType::MaxIndex> ValidSurfaceTypeAssets;
	TVoxelFixedBitArray<FVoxelSurfaceType::MaxIndex> ValidSmartSurfaceTypes;

public:
	FVoxelSurfaceTypeManager()
	{
		SurfaceTypeAssets_RequiresLock.Add(nullptr);
		SmartSurfaceTypes_RequiresLock.Add(nullptr);

		ValidSurfaceTypeAssets.Add(false);
		ValidSmartSurfaceTypes.Add(false);
	}

	void OnSurfaceTypeRegistered(const FVoxelSurfaceType SurfaceType) const
	{
		ensureVoxelSlow(!CriticalSection.IsLocked_Write());

		Voxel::GameTask([SurfaceType]
		{
#if !UE_BUILD_SHIPPING
			if (!GVoxelDebugSurfaceTypes.IsValidIndex(SurfaceType.RawValue))
			{
				GVoxelDebugSurfaceTypes.SetNum(SurfaceType.RawValue + 1);
			}

			GVoxelDebugSurfaceTypes[SurfaceType.RawValue] = SurfaceType.GetSurfaceTypeInterface();
#endif

			FVoxelInvalidationScope Scope("AddSurface " + SurfaceType.GetName());

			FVoxelSurfaceTypeTable::Refresh();
		});
	}

public:
	//~ Begin FVoxelSingleton Interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		VOXEL_FUNCTION_COUNTER();
		VOXEL_SCOPE_WRITE_LOCK(CriticalSection);

		for (auto It = SurfaceTypeAssetToIndex_RequiresLock.CreateIterator(); It; ++It)
		{
			TObjectPtr<UVoxelSurfaceTypeAsset> Type = It.Key();
			Collector.AddReferencedObject(Type);

			if (Type)
			{
				continue;
			}

			checkVoxelSlow(ValidSurfaceTypeAssets[It.Value()]);
			ValidSurfaceTypeAssets[It.Value()] = false;

			It.RemoveCurrent();
		}

		for (auto It = SmartSurfaceTypeToIndex_RequiresLock.CreateIterator(); It; ++It)
		{
			TObjectPtr<UVoxelSmartSurfaceType> Type = It.Key();
			Collector.AddReferencedObject(Type);

			if (Type)
			{
				continue;
			}

			checkVoxelSlow(ValidSmartSurfaceTypes[It.Value()]);
			ValidSmartSurfaceTypes[It.Value()] = false;

			It.RemoveCurrent();
		}
	}
	//~ End FVoxelSingleton Interface
};
FVoxelSurfaceTypeManager* GVoxelSurfaceTypeManager = new FVoxelSurfaceTypeManager();

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelSurfaceType::FVoxelSurfaceType(UVoxelSurfaceTypeInterface* SurfaceTypeInterface)
{
	VOXEL_FUNCTION_COUNTER();
	checkUObjectAccess();

	if (!SurfaceTypeInterface)
	{
		return;
	}

	if (UVoxelSurfaceTypeAsset* SurfaceTypeAsset = Cast<UVoxelSurfaceTypeAsset>(SurfaceTypeInterface))
	{
		InternalType = uint16(EClass::SurfaceTypeAsset);

		{
			VOXEL_SCOPE_READ_LOCK(GVoxelSurfaceTypeManager->CriticalSection);

			if (const uint16* IndexPtr = GVoxelSurfaceTypeManager->SurfaceTypeAssetToIndex_RequiresLock.Find(SurfaceTypeAsset))
			{
				InternalIndex = *IndexPtr;
				return;
			}
		}

		{
			VOXEL_SCOPE_WRITE_LOCK(GVoxelSurfaceTypeManager->CriticalSection);

			if (const uint16* IndexPtr = GVoxelSurfaceTypeManager->SurfaceTypeAssetToIndex_RequiresLock.Find(SurfaceTypeAsset))
			{
				InternalIndex = *IndexPtr;
				return;
			}

			check(GVoxelSurfaceTypeManager->SurfaceTypeAssets_RequiresLock.Num() < MaxIndex);
			InternalIndex = uint16(GVoxelSurfaceTypeManager->SurfaceTypeAssets_RequiresLock.Add(SurfaceTypeAsset));

			GVoxelSurfaceTypeManager->SurfaceTypeAssetToIndex_RequiresLock.Add_EnsureNew(SurfaceTypeAsset, uint16(InternalIndex));
			ensure(GVoxelSurfaceTypeManager->ValidSurfaceTypeAssets.Add(true) == InternalIndex);
		}

		GVoxelSurfaceTypeManager->OnSurfaceTypeRegistered(*this);
		return;
	}

	if (UVoxelSmartSurfaceType* SmartSurfaceType = Cast<UVoxelSmartSurfaceType>(SurfaceTypeInterface))
	{
		InternalType = uint16(EClass::SmartSurfaceType);

		{
			VOXEL_SCOPE_READ_LOCK(GVoxelSurfaceTypeManager->CriticalSection);

			if (const uint16* IndexPtr = GVoxelSurfaceTypeManager->SmartSurfaceTypeToIndex_RequiresLock.Find(SmartSurfaceType))
			{
				InternalIndex = *IndexPtr;
				return;
			}
		}

		{
			VOXEL_SCOPE_WRITE_LOCK(GVoxelSurfaceTypeManager->CriticalSection);

			if (const uint16* IndexPtr = GVoxelSurfaceTypeManager->SmartSurfaceTypeToIndex_RequiresLock.Find(SmartSurfaceType))
			{
				InternalIndex = *IndexPtr;
				return;
			}

			check(GVoxelSurfaceTypeManager->SmartSurfaceTypes_RequiresLock.Num() < MaxIndex);
			InternalIndex = uint16(GVoxelSurfaceTypeManager->SmartSurfaceTypes_RequiresLock.Add(SmartSurfaceType));

			GVoxelSurfaceTypeManager->SmartSurfaceTypeToIndex_RequiresLock.Add_EnsureNew(SmartSurfaceType, uint16(InternalIndex));
			ensure(GVoxelSurfaceTypeManager->ValidSmartSurfaceTypes.Add(true) == InternalIndex);
		}

		GVoxelSurfaceTypeManager->OnSurfaceTypeRegistered(*this);
		return;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelObjectPtr<UVoxelSurfaceTypeAsset> FVoxelSurfaceType::GetSurfaceTypeAsset() const
{
	if (IsNull() ||
		!ensureVoxelSlow(GetClass() == EClass::SurfaceTypeAsset))
	{
		return {};
	}

	VOXEL_SCOPE_READ_LOCK(GVoxelSurfaceTypeManager->CriticalSection);
	return GVoxelSurfaceTypeManager->SurfaceTypeAssets_RequiresLock[InternalIndex];
}

TVoxelObjectPtr<UVoxelSmartSurfaceType> FVoxelSurfaceType::GetSmartSurfaceType() const
{
	if (IsNull() ||
		!ensureVoxelSlow(GetClass() == EClass::SmartSurfaceType))
	{
		return {};
	}

	VOXEL_SCOPE_READ_LOCK(GVoxelSurfaceTypeManager->CriticalSection);
	return GVoxelSurfaceTypeManager->SmartSurfaceTypes_RequiresLock[InternalIndex];
}

TVoxelObjectPtr<UVoxelSurfaceTypeInterface> FVoxelSurfaceType::GetSurfaceTypeInterface() const
{
	if (GetClass() == EClass::SurfaceTypeAsset)
	{
		return GetSurfaceTypeAsset();
	}
	else
	{
		checkVoxelSlow(GetClass() == EClass::SmartSurfaceType);
		return GetSmartSurfaceType();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FName FVoxelSurfaceType::GetFName() const
{
	return GetSurfaceTypeInterface().GetFName();
}

FString FVoxelSurfaceType::GetName() const
{
	return GetSurfaceTypeInterface().GetName();
}

FLinearColor FVoxelSurfaceType::GetDebugColor() const
{
	return FLinearColor::IntToDistinctColor(RawValue, 1.f, 0.75f, 90.f);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelSurfaceType::ForeachSurfaceType(const TFunctionRef<void(FVoxelSurfaceType)> Lambda)
{
	VOXEL_FUNCTION_COUNTER();

	GVoxelSurfaceTypeManager->ValidSurfaceTypeAssets.ForAllSetBits([&](const int32 Index)
	{
		FVoxelSurfaceType SurfaceType;
		SurfaceType.InternalType = uint16(EClass::SurfaceTypeAsset);
		SurfaceType.InternalIndex = uint16(Index);
		Lambda(SurfaceType);
	});

	GVoxelSurfaceTypeManager->ValidSmartSurfaceTypes.ForAllSetBits([&](const int32 Index)
	{
		FVoxelSurfaceType SurfaceType;
		SurfaceType.InternalType = uint16(EClass::SmartSurfaceType);
		SurfaceType.InternalIndex = uint16(Index);
		Lambda(SurfaceType);
	});
}