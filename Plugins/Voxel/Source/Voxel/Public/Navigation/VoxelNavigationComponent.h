// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNavigationComponent.generated.h"

class FVoxelNavigationMesh;

UCLASS()
class VOXEL_API UVoxelNavigationComponent final : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UVoxelNavigationComponent();

	TSharedPtr<const FVoxelNavigationMesh> GetNavigationMesh() const
	{
		return NavigationMesh;
	}

	void SetNavigationMesh(const TSharedPtr<const FVoxelNavigationMesh>& NewNavigationMesh);

public:
	//~ Begin UPrimitiveComponent Interface
	virtual bool ShouldCreatePhysicsState() const override { return false; }
	virtual bool IsNavigationRelevant() const override;
	virtual bool DoCustomNavigableGeometryExport(FNavigableGeometryExport& GeomExport) const override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	//~ End UPrimitiveComponent Interface

private:
	TSharedPtr<const FVoxelNavigationMesh> NavigationMesh;
};