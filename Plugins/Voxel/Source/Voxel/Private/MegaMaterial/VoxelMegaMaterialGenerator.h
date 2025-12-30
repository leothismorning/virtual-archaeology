// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "MaterialExpressionIO.h"
#include "MegaMaterial/VoxelMegaMaterialGeneratedData.h"

#if WITH_EDITOR
class FVoxelMegaMaterialGenerator
{
public:
	static bool GenerateMaterialForTarget(
		const UVoxelMegaMaterial& MegaMaterial,
		const TMap<FVoxelMaterialRenderIndex, FVoxelMegaMaterialSurfaceInfo>& IndexToSurfaceInfo,
		EVoxelMegaMaterialTarget Target,
		UMaterial& NewMaterial,
		UMaterialInstanceConstant& NewMaterialInstance,
		TConstVoxelArrayView<TObjectPtr<UVoxelMetadata>> Metadatas,
		TVoxelSet<TVoxelObjectPtr<UObject>>& OutWatchedMaterials,
		bool bIsDummyMaterial);

	static bool GenerateMaterial(
		const UVoxelMegaMaterial& MegaMaterial,
		const UVoxelSurfaceTypeAsset& SurfaceType,
		UMaterial& NewMaterial,
		UMaterialInstanceConstant& NewMaterialInstance,
		TConstVoxelArrayView<TObjectPtr<UVoxelMetadata>> Metadatas,
		bool bIsDummyMaterial);

	static bool ApplyBlendSmoothness(
		const TMap<FVoxelMaterialRenderIndex, FVoxelMegaMaterialSurfaceInfo>& IndexToSurfaceInfo,
		const UMaterial& Material,
		UMaterialInstanceConstant& MaterialInstance);

public:
	static TVoxelArray<UVoxelMetadata*> GetUsedMetadatas(const UMaterial& Material);
	static TVoxelArray<UVoxelMetadata*> GetUsedMetadatas(const UMaterialFunction& MaterialFunction);

private:
	static bool AddAttributePostProcess(
		const UVoxelMegaMaterial& MegaMaterial,
		UMaterial& Material);

	static void CollectedWatchedMaterials(
		UMaterialInterface& Material,
		TVoxelSet<TVoxelObjectPtr<UObject>>& OutWatchedMaterials);

	static void WrapInComment(
		UMaterial& Material,
		const FVoxelIntBox2D& Bounds,
		const FString& Comment);

	static bool ShouldDuplicateFunction(const UMaterialExpression& Expression);
};
#endif