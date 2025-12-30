// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Surface/VoxelSurfaceType.h"
#include "MegaMaterial/VoxelRenderMaterial.h"

struct FVoxelSubsystem;
class FVoxelNaniteMesh;
class FVoxelMegaMaterialProxy;
class FVoxelNaniteMaterialRendererImpl;

class FVoxelNaniteMaterialRenderer : public TSharedFromThis<FVoxelNaniteMaterialRenderer>
{
public:
	explicit FVoxelNaniteMaterialRenderer(const TSharedRef<FVoxelMegaMaterialProxy>& MegaMaterialProxy);

public:
	TSharedPtr<FVoxelMaterialInstanceRef> GetMaterialInstance(FVoxelMaterialRenderIndex RenderIndex) const;

	void AddReferencedObjects(FReferenceCollector& Collector);

	void PrepareRender(TVoxelSet<TSharedPtr<const FVoxelNaniteMesh>>&& NewMeshes);

	void UpdateRender(
		const FVoxelSubsystem& Subsystem,
		const FTransform& NewLocalToWorld);

private:
	const TSharedRef<FVoxelNaniteMaterialRendererImpl> Impl;

	TVoxelSet<TSharedPtr<const FVoxelNaniteMesh>> Meshes;
	TVoxelArray<FVoxelSurfaceType> UsedSurfaceTypes;
	TVoxelArray<int32> PageToOffset;
	TObjectPtr<UTexture2D> PageToOffset_Texture;
};