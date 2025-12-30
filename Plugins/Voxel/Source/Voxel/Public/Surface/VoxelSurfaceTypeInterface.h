// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelSurfaceTypeInterface.generated.h"

class UVoxelSurfaceTypeAsset;

UCLASS(Abstract)
class VOXEL_API UVoxelSurfaceTypeInterface : public UVoxelAsset
{
	GENERATED_BODY()

public:
	//~ Begin UObject Interface
	virtual void Serialize(FArchive& Ar) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface

public:
#if WITH_EDITOR
	static UVoxelSurfaceTypeAsset* Migrate(UObject* Material);

	static void Migrate(
		TObjectPtr<UObject>& Material,
		TObjectPtr<UVoxelSurfaceTypeInterface>& SurfaceType);

	static void Migrate(
		TArray<TObjectPtr<UMaterialInterface>>& Materials,
		TArray<TObjectPtr<UVoxelSurfaceTypeAsset>>& SurfaceTypes);
#endif
};