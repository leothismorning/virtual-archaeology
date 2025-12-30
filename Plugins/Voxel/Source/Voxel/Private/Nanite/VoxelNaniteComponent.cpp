// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nanite/VoxelNaniteComponent.h"
#include "Nanite/VoxelNaniteMesh.h"
#include "Nanite/VoxelNaniteMaterialRenderer.h"
#include "MegaMaterial/VoxelMegaMaterialProxy.h"
#include "VoxelMesh.h"

#include "NaniteSceneProxy.h"
#include "StaticMeshResources.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

UVoxelNaniteComponent::UVoxelNaniteComponent()
{
	BodyInstance.SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void UVoxelNaniteComponent::SetMesh(
	const TSharedPtr<const FVoxelNaniteMesh>& NewMesh,
	const FVoxelMegaMaterialProxy& MegaMaterialProxy,
	const TSharedPtr<FVoxelNaniteMaterialRenderer>& MaterialRenderer)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	ensure(Mesh != NewMesh);
	Mesh = NewMesh;

	for (int32 Index = 1; Index < OverrideMaterials.Num(); Index++)
	{
		SetMaterial(Index, nullptr);
	}

	if (!Mesh)
	{
		SetStaticMesh(nullptr);
		return;
	}

	ensure(Mesh->StaticMesh);
	SetStaticMesh(Mesh->StaticMesh);

	ensure(2 + Mesh->Mesh->UsedSurfaceTypes.Num() == GetNumMaterials());
	ensure(2 + Mesh->Mesh->UsedSurfaceTypes.Num() == Mesh->StaticMesh->GetRenderData()->LODResources[0].Sections.Num());

	if (!ensure(MaterialRenderer))
	{
		return;
	}

	int32 UsedMaterialIndex = 0;

	const auto AddUsedMaterial = [&](const FVoxelMaterialRenderIndex RenderIndex)
	{
		const TSharedPtr<FVoxelMaterialInstanceRef> Instance = MaterialRenderer->GetMaterialInstance(RenderIndex);
		if (!ensureVoxelSlow(Instance))
		{
			return;
		}

		UMaterialInterface* Material = Instance->GetInstance();
		if (!ensureVoxelSlow(Material))
		{
			return;
		}

		if (Material->GetBlendMode() == BLEND_Translucent)
		{
			// Otherwise the entire mesh won't render
			Material = nullptr;
		}

		SetMaterial(1 + UsedMaterialIndex, Material);

		UsedMaterialIndex++;
	};

	// Always mark the default material as used
	AddUsedMaterial({});

	for (const FVoxelSurfaceType SurfaceType : Mesh->Mesh->UsedSurfaceTypes)
	{
		AddUsedMaterial(MegaMaterialProxy.GetRenderIndex(SurfaceType));
	}
}

void UVoxelNaniteComponent::SetNaniteMaterial(UMaterialInterface* Material)
{
	SetMaterial(0, Material);
}

bool UVoxelNaniteComponent::ShouldCreatePhysicsState() const
{
	return false;
}

void UVoxelNaniteComponent::OnComponentDestroyed(const bool bDestroyingHierarchy)
{
	VOXEL_FUNCTION_COUNTER();

	Super::OnComponentDestroyed(bDestroyingHierarchy);

	// Clear memory
	Mesh.Reset();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class VOXEL_API FVoxelNaniteSceneProxy : public Nanite::FSceneProxy
{
public:
	// Make sure to keep the mesh alive while the proxy is alive
	TSharedPtr<const FVoxelNaniteMesh> Mesh;

	using FSceneProxy::FSceneProxy;
	UE_NONCOPYABLE(FVoxelNaniteSceneProxy);
};

FPrimitiveSceneProxy* UVoxelNaniteComponent::CreateStaticMeshSceneProxy(
	Nanite::FMaterialAudit& NaniteMaterials,
	const bool bCreateNanite)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensureVoxelSlow(bCreateNanite))
	{
		return nullptr;
	}

	extern bool GVoxelShowRaytracedMeshes;
	extern bool GVoxelShowDistanceFieldMeshes;

	if (GVoxelShowRaytracedMeshes ||
		GVoxelShowDistanceFieldMeshes)
	{
		return nullptr;
	}

	FVoxelNaniteSceneProxy* Proxy = new FVoxelNaniteSceneProxy(NaniteMaterials, this);
	Proxy->Mesh = Mesh;
	return Proxy;
}