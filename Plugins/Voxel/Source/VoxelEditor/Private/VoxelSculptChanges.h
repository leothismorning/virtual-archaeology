// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Misc/Change.h"

class FVoxelHeightSculptInnerData;
class FVoxelVolumeSculptInnerData;

class FVoxelHeightSculptChange : public FSwapChange
{
public:
	const TSharedRef<const FVoxelHeightSculptInnerData> Snapshot;

	explicit FVoxelHeightSculptChange(const TSharedRef<const FVoxelHeightSculptInnerData>& Snapshot)
		: Snapshot(Snapshot)
	{
	}

	//~ Begin FSwapChange Interface
	virtual FString ToString() const override;
	virtual TUniquePtr<FChange> Execute(UObject* Object) override;
	//~ End FSwapChange Interface
};

class FVoxelVolumeSculptChange : public FSwapChange
{
public:
	const TSharedRef<const FVoxelVolumeSculptInnerData> Snapshot;

	explicit FVoxelVolumeSculptChange(const TSharedRef<const FVoxelVolumeSculptInnerData>& Snapshot)
		: Snapshot(Snapshot)
	{
	}

	//~ Begin FSwapChange Interface
	virtual FString ToString() const override;
	virtual TUniquePtr<FChange> Execute(UObject* Object) override;
	//~ End FSwapChange Interface
};