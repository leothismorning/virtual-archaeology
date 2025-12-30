// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Collision/VoxelCollisionComponent.h"
#include "Collision/VoxelCollider.h"
#include "Surface/VoxelSurfaceTypeAsset.h"
#include "VoxelChaosTriangleMeshSceneProxy.h"
#include "Materials/Material.h"
#include "PhysicsEngine/BodySetup.h"
#include "Chaos/TriangleMeshImplicitObject.h"

void UVoxelCollisionComponent::SetBodyInstance(const FBodyInstance& NewBodyInstance)
{
	FVoxelUtilities::CopyBodyInstance(
		BodyInstance,
		NewBodyInstance);
}

void UVoxelCollisionComponent::SetCollider(const TSharedPtr<const FVoxelCollider>& NewCollider)
{
	VOXEL_FUNCTION_COUNTER();

	Collider = NewCollider;

	if (Collider)
	{
		Collider->UpdateStats();
	}

	if (BodySetup)
	{
		BodySetup->ClearPhysicsMeshes();
	}
	else
	{
		BodySetup = NewObject<UBodySetup>(this);
		BodySetup->bGenerateMirroredCollision = false;
		BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
	}

	if (Collider)
	{
		ensure(BodySetup->TriMeshGeometries.Num() == 0);

		// Copied from UBodySetup::FinishCreatingPhysicsMeshes_Chaos
#if TRACK_CHAOS_GEOMETRY
		Collider->TriangleMesh->Track(MakeSerializable(Collider->TriangleMesh), "Voxel Mesh");
#endif
		ensure(!Collider->TriangleMesh->GetDoCollide());

		BodySetup->TriMeshGeometries.Add(Collider->TriangleMesh);
		BodySetup->bCreatedPhysicsMeshes = true;
	}

	BodyInstance.OwnerComponent = this;
	BodyInstance.UpdatePhysicalMaterials();

	RecreatePhysicsState();
	MarkRenderStateDirty();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelCollisionComponent::ShouldCreatePhysicsState() const
{
	return Collider.IsValid();
}

FBoxSphereBounds UVoxelCollisionComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	const FBox LocalBounds =
		Collider.IsValid()
		? FBox(Collider->TriangleMesh->BoundingBox().Min(), Collider->TriangleMesh->BoundingBox().Max())
		: FBox(FVector::ZeroVector, FVector::ZeroVector);

	ensure(!LocalBounds.Min.ContainsNaN());
	ensure(!LocalBounds.Max.ContainsNaN());

	return LocalBounds.TransformBy(LocalToWorld);
}

void UVoxelCollisionComponent::OnComponentDestroyed(const bool bDestroyingHierarchy)
{
	VOXEL_FUNCTION_COUNTER();

	Super::OnComponentDestroyed(bDestroyingHierarchy);

	// Clear memory
	Collider.Reset();

	if (BodySetup)
	{
		BodySetup->ClearPhysicsMeshes();
	}
}

FPrimitiveSceneProxy* UVoxelCollisionComponent::CreateSceneProxy()
{
	if (!GIsEditor ||
		!Collider)
	{
		return nullptr;
	}

	return new FVoxelChaosTriangleMeshSceneProxy(*this, Collider->TriangleMesh);
}

int32 UVoxelCollisionComponent::GetNumMaterials() const
{
	if (!Collider)
	{
		return 0;
	}

	return Collider->SurfaceTypes.Num();
}

UMaterialInterface* UVoxelCollisionComponent::GetMaterial(const int32 ElementIndex) const
{
	if (!Collider)
	{
		return nullptr;
	}

	if (!ensure(Collider->SurfaceTypes.IsValidIndex(ElementIndex)))
	{
		return nullptr;
	}

	const UVoxelSurfaceTypeAsset* SurfaceType = Collider->SurfaceTypes[ElementIndex].Resolve();
	if (!SurfaceType)
	{
		return nullptr;
	}

	return SurfaceType->Material;
}