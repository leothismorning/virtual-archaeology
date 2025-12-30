// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelExposedSeed.h"
#include "Surface/VoxelSurfaceTypeInterface.h"
#include "VoxelSurfaceTypeAsset.generated.h"

UCLASS(BlueprintType, meta = (VoxelAssetType, AssetColor=Green))
class VOXEL_API UVoxelSurfaceTypeAsset : public UVoxelSurfaceTypeInterface
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	TObjectPtr<UMaterialInterface> Material;

	// Useful to create holes in the terrain
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bInvisible = false;

	// Controls the smooth blend behavior
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta=(UIMin = 0, UIMax = 1, ClampMin = 0.f))
	float BlendSmoothness = 0.f;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Config")
	FVoxelExposedSeed Seed;

public:
	UPROPERTY()
	TObjectPtr<UObject> LegacyMaterial;

public:
	//~ Begin UObject Interface
	virtual void PostInitProperties() override;
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface
};