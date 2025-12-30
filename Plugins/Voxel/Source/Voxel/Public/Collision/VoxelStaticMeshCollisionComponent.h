// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "VoxelStaticMeshCollisionComponent.generated.h"

class FVoxelCollider;

UCLASS()
class VOXEL_API UVoxelStaticMeshCollisionComponent final : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UVoxelStaticMeshCollisionComponent() = default;

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
	//~ End UPrimitiveComponent Interface

private:
	UPROPERTY(Transient)
	TObjectPtr<UBodySetup> BodySetup;

	TSharedPtr<const FVoxelCollider> Collider;
};