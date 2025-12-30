// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelSubsystem.h"
#include "MegaMaterial/VoxelMegaMaterialTarget.h"
#include "VoxelMaterialSubsystem.generated.h"

USTRUCT()
struct VOXEL_API FVoxelMaterialSubsystem : public FVoxelSubsystem
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_BODY()

public:
	TSharedRef<FVoxelMaterialInstanceRef> GetMaterialInstanceRef(EVoxelMegaMaterialTarget Target) const;

	//~ Begin FVoxelSubsystem Interface
	virtual bool ShouldCreateOnServer() const override { return false; }
	virtual void LoadFromPrevious(FVoxelSubsystem& InPreviousSubsystem) override;
	virtual void Initialize() override;
	virtual void Render(FVoxelRuntime& Runtime) override;
	//~ End FVoxelSubsystem Interface

private:
	TVoxelStaticArray<TSharedPtr<FVoxelMaterialInstanceRef>, int32(EVoxelMegaMaterialTarget::Max)> MaterialInstanceRefs;
};