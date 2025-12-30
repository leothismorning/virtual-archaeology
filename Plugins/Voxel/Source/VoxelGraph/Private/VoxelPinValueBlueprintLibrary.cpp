// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelPinValueBlueprintLibrary.h"

DEFINE_FUNCTION(UVoxelPinValueBlueprintLibrary::execK2_MakeVoxelPinValue)
{
	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;

	Stack.StepCompiledIn<FProperty>(nullptr);

	const FProperty* Property = Stack.MostRecentProperty;
	const void* PropertyAddress = Stack.MostRecentPropertyAddress;

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;

	Stack.StepCompiledIn<FProperty>(nullptr);

	P_FINISH;

	if (!ensure(Property))
	{
		VOXEL_MESSAGE(Error, "Invalid value");
		return;
	}

	const FVoxelPinValue Value = FVoxelPinValue::MakeFromProperty(*Property, PropertyAddress);;
	if (!Value.IsValid())
	{
		VOXEL_MESSAGE(Error, "Invalid value");
		return;
	}

	P_NATIVE_BEGIN;
	*(FVoxelPinValue*)RESULT_PARAM = Value;
	P_NATIVE_END;
}