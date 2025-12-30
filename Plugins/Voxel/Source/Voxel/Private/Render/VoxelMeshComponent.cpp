// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Render/VoxelMeshComponent.h"
#include "VoxelMesh.h"
#include "VoxelMeshSceneProxy.h"
#include "VoxelMeshRenderProxy.h"
#include "Materials/MaterialInterface.h"

UVoxelMeshComponent::UVoxelMeshComponent()
{
	CastShadow = true;
	bUseAsOccluder = true;

	BodyInstance.SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void UVoxelMeshComponent::SetRenderProxy(
	const TSharedRef<FVoxelMeshRenderProxy>& NewRenderProxy,
	const TSharedRef<FVoxelMaterialRef>& NewMaterialRef,
	const TSharedRef<FVoxelMaterialRef>& NewLumenMaterialRef)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	ensureVoxelSlow(IsRegistered());

	RenderProxy = NewRenderProxy;
	MaterialRef = NewMaterialRef;
	LumenMaterialRef = NewLumenMaterialRef;

	bVisibleInRayTracing = NewRenderProxy->bEnableRaytracing;

	RuntimeVirtualTextures.Reset();

	for (const TVoxelObjectPtr<URuntimeVirtualTexture>& RuntimeVirtualTexture : NewRenderProxy->RuntimeVirtualTextures)
	{
		RuntimeVirtualTextures.Add(RuntimeVirtualTexture.Resolve_Ensured());
	}

	MarkRenderStateDirty();

#if WITH_EDITOR
	FVoxelUtilities::EnsureViewportIsUpToDate();
#endif
}

void UVoxelMeshComponent::ClearRenderProxy()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	ensureVoxelSlow(IsRegistered());

	RenderProxy.Reset();
	MaterialRef.Reset();
	LumenMaterialRef.Reset();
	RuntimeVirtualTextures.Reset();

	MarkRenderStateDirty();

#if WITH_EDITOR
	FVoxelUtilities::EnsureViewportIsUpToDate();
#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FPrimitiveSceneProxy* UVoxelMeshComponent::CreateSceneProxy()
{
	VOXEL_FUNCTION_COUNTER();

	if (!MaterialRef ||
		!RenderProxy)
	{
		return nullptr;
	}

	return new FVoxelMeshSceneProxy(*this);
}

void UVoxelMeshComponent::GetUsedMaterials(
	TArray<UMaterialInterface*>& OutMaterials,
	const bool bGetDebugMaterials) const
{
	Super::GetUsedMaterials(OutMaterials, bGetDebugMaterials);

	if (MaterialRef)
	{
		OutMaterials.Add(MaterialRef->GetMaterial());
	}

	if (LumenMaterialRef)
	{
		OutMaterials.Add(LumenMaterialRef->GetMaterial());
	}
}

FBoxSphereBounds UVoxelMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	const FVoxelBox LocalBounds = RenderProxy ? RenderProxy->Mesh->Bounds : FVoxelBox();
	ensure(LocalBounds.IsValid());
	return LocalBounds.TransformBy(LocalToWorld).ToFBox();
}

void UVoxelMeshComponent::OnComponentDestroyed(const bool bDestroyingHierarchy)
{
	VOXEL_FUNCTION_COUNTER();

	Super::OnComponentDestroyed(bDestroyingHierarchy);

	// Clear memory
	RenderProxy.Reset();
}