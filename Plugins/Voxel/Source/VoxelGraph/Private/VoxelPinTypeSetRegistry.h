// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelPinType.h"
#if WITH_EDITOR
#include "Kismet2/EnumEditorUtils.h"
#endif

#if WITH_EDITOR
struct FVoxelPinTypeSetRegistry
	: public FVoxelSingleton
	, public FEnumEditorUtils::INotifyOnEnumChanged
{
public:
	const TVoxelSet<FVoxelPinType>& GetTypes();

public:
	//~ Begin INotifyOnEnumChanged Interface
	virtual void PreChange(
		const UUserDefinedEnum* Changed,
		FEnumEditorUtils::EEnumEditorChangeInfo ChangedType) override;

	virtual void PostChange(
		const UUserDefinedEnum* Changed,
		FEnumEditorUtils::EEnumEditorChangeInfo ChangedType) override;
	//~ End INotifyOnEnumChanged Interface

private:
	TVoxelSet<FVoxelPinType> Types;
	TVoxelSet<FVoxelPinType> TypesWithUserEnums;

	void InitializeTypes();
	void InitializeTypesWithUserEnums();

private:
	void RunTests() const;
};
#endif

#if WITH_EDITOR
extern FVoxelPinTypeSetRegistry* GVoxelPinTypeSetRegistry;
#endif