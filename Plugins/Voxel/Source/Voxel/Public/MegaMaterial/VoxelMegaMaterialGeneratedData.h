// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelRenderMaterial.h"
#include "VoxelMegaMaterialTarget.h"
#include "VoxelMegaMaterialGeneratedData.generated.h"

class UVoxelMetadata;
class UVoxelMegaMaterial;
class UVoxelSurfaceTypeAsset;

USTRUCT()
struct FVoxelMegaMaterialGeneratedMaterial
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UMaterial> Material;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceConstant> Instance;
};

USTRUCT()
struct FVoxelMegaMaterialSurfaceInfo
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UVoxelSurfaceTypeAsset> SurfaceType;

	UPROPERTY()
	TArray<TObjectPtr<UVoxelMetadata>> UsedMetadatas;
};

UCLASS()
class VOXEL_API UVoxelMegaMaterialGeneratedData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TMap<FVoxelMaterialRenderIndex, FVoxelMegaMaterialSurfaceInfo> IndexToSurfaceInfo;

	UPROPERTY()
	TMap<FVoxelMaterialRenderIndex, FVoxelMegaMaterialGeneratedMaterial> IndexToGeneratedMaterial;

	UPROPERTY()
	TArray<TObjectPtr<UVoxelMetadata>> MetadataIndexToMetadata;

	UPROPERTY()
	TMap<EVoxelMegaMaterialTarget, TObjectPtr<UMaterialInterface>> TargetToMaterial;

public:
	UVoxelMegaMaterial* GetMegaMaterial();
	void SetMegaMaterial(UVoxelMegaMaterial* NewMegaMaterial);

	//~ Begin UObject Interface
	virtual void Serialize(FArchive& Ar) override;
	//~ End UObject Interface

public:
#if WITH_EDITOR
	void ForceRebuild();
	void QueueRebuild(bool bInteractive = false);
	bool IsRebuildQueued() const;
	void RebuildNow(bool bInteractive = false);

	static void OnMaterialChanged(
		UMaterialInterface* Material,
		bool bInteractive = false);
	
	static void OnMaterialFunctionChanged(UMaterialFunction* MaterialFunction);
#endif

private:
	UPROPERTY()
	TSoftObjectPtr<UVoxelMegaMaterial> SoftMegaMaterial;

	UPROPERTY()
	TMap<EVoxelMegaMaterialTarget, FVoxelMegaMaterialGeneratedMaterial> TargetToGeneratedMaterial;

	TVoxelObjectPtr<UVoxelMegaMaterial> WeakMegaMaterial;
	TVoxelSet<TVoxelObjectPtr<UObject>> WatchedMaterials;

#if WITH_EDITOR
	void GenerateMaterialForTarget(
		const UVoxelMegaMaterial& MegaMaterial,
		EVoxelMegaMaterialTarget Target,
		bool bInteractive);

	void GenerateMaterial(
		const UVoxelMegaMaterial& MegaMaterial,
		const UVoxelSurfaceTypeAsset& SurfaceType,
		FVoxelMegaMaterialGeneratedMaterial& GeneratedMaterial,
		bool bInteractive);
#endif
};
