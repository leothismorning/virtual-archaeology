// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Navigation/VoxelNavigationComponent.h"
#include "Navigation/VoxelNavigationMesh.h"
#include "Navigation/VoxelNavigationMeshSceneProxy.h"
#include "AI/NavigationSystemBase.h"
#include "AI/NavigationSystemHelpers.h"

UVoxelNavigationComponent::UVoxelNavigationComponent()
{
	bCanEverAffectNavigation = true;
	bHasCustomNavigableGeometry = EHasCustomNavigableGeometry::EvenIfNotCollidable;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelNavigationComponent::SetNavigationMesh(const TSharedPtr<const FVoxelNavigationMesh>& NewNavigationMesh)
{
	VOXEL_FUNCTION_COUNTER();

	NavigationMesh = NewNavigationMesh;

	if (NavigationMesh)
	{
		ensure(!NavigationMesh->IsEmpty());
		NavigationMesh->UpdateStats();
	}

	UpdateBounds();
	MarkRenderStateDirty();

	if (IsRegistered() &&
		GetWorld() &&
		GetWorld()->GetNavigationSystem() &&
		FNavigationSystem::WantsComponentChangeNotifies())
	{
		VOXEL_SCOPE_COUNTER("UpdateComponentData");

		bNavigationRelevant = IsNavigationRelevant();
		FNavigationSystem::UpdateComponentData(*this);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelNavigationComponent::IsNavigationRelevant() const
{
	return NavigationMesh.IsValid();
}

bool UVoxelNavigationComponent::DoCustomNavigableGeometryExport(FNavigableGeometryExport& GeomExport) const
{
	VOXEL_FUNCTION_COUNTER();

	if (NavigationMesh)
	{
		const TArray<FVector> DoubleVertices(NavigationMesh->Vertices);

		GeomExport.ExportCustomMesh(
			DoubleVertices.GetData(),
			NavigationMesh->Vertices.Num(),
			NavigationMesh->Indices.GetData(),
			NavigationMesh->Indices.Num(),
			GetComponentTransform());
	}

	return false;
}

FBoxSphereBounds UVoxelNavigationComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	const FVoxelBox LocalBounds = NavigationMesh ? NavigationMesh->LocalBounds : FVoxelBox();
	ensure(LocalBounds.IsValid());
	return LocalBounds.TransformBy(LocalToWorld).ToFBox();
}

void UVoxelNavigationComponent::OnComponentDestroyed(const bool bDestroyingHierarchy)
{
	VOXEL_FUNCTION_COUNTER();

	Super::OnComponentDestroyed(bDestroyingHierarchy);

	// Clear memory
	NavigationMesh.Reset();
}

FPrimitiveSceneProxy* UVoxelNavigationComponent::CreateSceneProxy()
{
	if (!GIsEditor ||
		!NavigationMesh)
	{
		return nullptr;
	}

	return new FVoxelNavigationMeshSceneProxy(*this, NavigationMesh.ToSharedRef());
}