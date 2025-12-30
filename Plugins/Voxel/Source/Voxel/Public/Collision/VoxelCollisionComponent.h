// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelCollisionComponent.generated.h"

class FVoxelCollider;

UCLASS()
class VOXEL_API UVoxelCollisionComponent final : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UVoxelCollisionComponent() = default;

	TSharedPtr<const FVoxelCollider> GetCollider() const
	{
		return Collider;
	}

	void SetBodyInstance(const FBodyInstance& NewBodyInstance);
	void SetCollider(const TSharedPtr<const FVoxelCollider>& NewCollider);

public:
	//~ Begin UPrimitiveComponent Interface
	virtual UBodySetup* GetBodySetup() override { return BodySetup; }
	virtual bool ShouldCreatePhysicsState() const override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual int32 GetNumMaterials() const override;
	virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
	//~ End UPrimitiveComponent Interface

private:
	UPROPERTY(Transient)
	TObjectPtr<UBodySetup> BodySetup;

	TSharedPtr<const FVoxelCollider> Collider;
};