// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelObjectPinType.h"

TVoxelMap<const UScriptStruct*, TSharedPtr<const FVoxelObjectPinType>> GVoxelStructToObjectPinType;

TVoxelArray<UClass*> FVoxelObjectPinType::GetAllowedClasses() const
{
	return { GetClass() };
}

UObject* FVoxelObjectPinType::GetObject(const FConstVoxelStructView Struct) const
{
	const TVoxelObjectPtr<UObject> WeakObject = GetWeakObject(Struct);
	ensureVoxelSlowNoSideEffects(WeakObject.IsValid_Slow() || WeakObject.IsExplicitlyNull());
	return WeakObject.Resolve();
}

void FVoxelObjectPinType::RegisterPinType(const TSharedRef<const FVoxelObjectPinType>& PinType)
{
	check(IsInGameThread());
	GVoxelStructToObjectPinType.Add_EnsureNew(PinType->GetStruct(), PinType);
}

const TVoxelMap<const UScriptStruct*, TSharedPtr<const FVoxelObjectPinType>>& FVoxelObjectPinType::StructToPinType()
{
	static bool bInitialized = false;

	if (!bInitialized)
	{
		VOXEL_FUNCTION_COUNTER();
		check(IsInGameThread());

		bInitialized = true;
		GVoxelStructToObjectPinType.Reserve(128);

		for (const UScriptStruct* Struct : GetDerivedStructs<FVoxelObjectPinType>())
		{
			const TSharedRef<FVoxelObjectPinType> Instance = MakeSharedStruct<FVoxelObjectPinType>(Struct);
			GVoxelStructToObjectPinType.Add_CheckNew(Instance->GetStruct(), Instance);
		}

		GVoxelStructToObjectPinType.Shrink();
	}

	return GVoxelStructToObjectPinType;
}