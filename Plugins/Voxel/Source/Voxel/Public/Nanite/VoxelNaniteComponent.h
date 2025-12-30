// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "VoxelNaniteComponent.generated.h"

class FVoxelNaniteMesh;
class FVoxelMegaMaterialProxy;
class FVoxelNaniteMaterialRenderer;

UCLASS(NotBlueprintable)
class VOXEL_API UVoxelNaniteComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UVoxelNaniteComponent();

	const TSharedPtr<const FVoxelNaniteMesh>& GetMesh() const
	{
		return Mesh;
	}

	void SetMesh(
		const TSharedPtr<const FVoxelNaniteMesh>& NewMesh,
		const FVoxelMegaMaterialProxy& MegaMaterialProxy,
		const TSharedPtr<FVoxelNaniteMaterialRenderer>& MaterialRenderer);

	void SetNaniteMaterial(UMaterialInterface* Material);

	//~ Begin UStaticMeshComponent Interface
	virtual bool ShouldCreatePhysicsState() const override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	virtual FPrimitiveSceneProxy* CreateStaticMeshSceneProxy(
		Nanite::FMaterialAudit& NaniteMaterials,
		bool bCreateNanite) override;
	//~ End UStaticMeshComponent Interface

private:
	TSharedPtr<const FVoxelNaniteMesh> Mesh;
};