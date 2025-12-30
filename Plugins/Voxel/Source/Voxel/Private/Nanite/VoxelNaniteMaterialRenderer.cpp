// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nanite/VoxelNaniteMaterialRenderer.h"
#include "Nanite/VoxelNaniteMesh.h"
#include "Nanite/VoxelNaniteMaterialRendererImpl.h"
#include "MegaMaterial/VoxelMaterialSubsystem.h"
#include "MegaMaterial/VoxelMegaMaterialProxy.h"
#include "VoxelMesh.h"
#include "VoxelSubsystem.h"
#include "VoxelBufferPool.h"

#include "TextureResource.h"
#include "MaterialCachedData.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"

FVoxelNaniteMaterialRenderer::FVoxelNaniteMaterialRenderer(const TSharedRef<FVoxelMegaMaterialProxy>& MegaMaterialProxy)
	: Impl(FVoxelNaniteMaterialRendererImpl::Create(MegaMaterialProxy))
{
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<FVoxelMaterialInstanceRef> FVoxelNaniteMaterialRenderer::GetMaterialInstance(const FVoxelMaterialRenderIndex RenderIndex) const
{
	return Impl->GetMaterialInstance(RenderIndex);
}

void FVoxelNaniteMaterialRenderer::AddReferencedObjects(FReferenceCollector& Collector)
{
	VOXEL_FUNCTION_COUNTER();

	Collector.AddReferencedObject(PageToOffset_Texture);
}

void FVoxelNaniteMaterialRenderer::PrepareRender(TVoxelSet<TSharedPtr<const FVoxelNaniteMesh>>&& NewMeshes)
{
	VOXEL_FUNCTION_COUNTER();

	Meshes = MoveTemp(NewMeshes);

	{
		TVoxelSet<FVoxelSurfaceType> NewUsedSurfaceTypes;
		NewUsedSurfaceTypes.Reserve(64);

		for (const TSharedPtr<const FVoxelNaniteMesh>& Mesh : Meshes)
		{
			NewUsedSurfaceTypes.Append(Mesh->Mesh->UsedSurfaceTypes);
		}

		UsedSurfaceTypes = NewUsedSurfaceTypes.Array();
	}
	UsedSurfaceTypes.AddUnique(FVoxelSurfaceType());
	UsedSurfaceTypes.Sort();

	ensure(PageToOffset.Num() == 0);
	FVoxelUtilities::SetNumFast(PageToOffset, 256 * 256);
	FVoxelUtilities::SetAll(PageToOffset, -1);

	const int32 NumAttributes = 1 + Impl->MegaMaterialProxy->NumMetadatas();

	for (const TSharedPtr<const FVoxelNaniteMesh>& Mesh : Meshes)
	{
		ensure(Mesh->AttributeIndicesBufferRef);

		for (const FVoxelNaniteMesh::FPage& Page : Mesh->Pages)
		{
			if (!ensure(PageToOffset.IsValidIndex(Page.Index)))
			{
				continue;
			}

			const int64 Index = Mesh->AttributeIndicesBufferRef->GetIndex() + Page.VertexOffset * NumAttributes;
			checkVoxelSlow(Index <= MAX_int32);
			PageToOffset[Page.Index] = Index;
		}
	}
}

void FVoxelNaniteMaterialRenderer::UpdateRender(
	const FVoxelSubsystem& Subsystem,
	const FTransform& NewLocalToWorld)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	{
		VOXEL_SCOPE_COUNTER("Update material instances");

		const FVoxelMaterialSubsystem& MaterialSubsystem = Subsystem.GetSubsystem<FVoxelMaterialSubsystem>();

		TVoxelArray<TSharedRef<FVoxelMaterialInstanceRef>> MaterialInstanceRefs;
		MaterialInstanceRefs.Add(MaterialSubsystem.GetMaterialInstanceRef(EVoxelMegaMaterialTarget::NaniteWPO));
		MaterialInstanceRefs.Add(MaterialSubsystem.GetMaterialInstanceRef(EVoxelMegaMaterialTarget::NaniteDisplacement));
		MaterialInstanceRefs.Add(MaterialSubsystem.GetMaterialInstanceRef(EVoxelMegaMaterialTarget::NaniteMaterialSelection));
		MaterialInstanceRefs.Append(Impl->MaterialIndexToMaterialInstance.ValueArray());

		if (!PageToOffset_Texture)
		{
			PageToOffset_Texture = FVoxelTextureUtilities::CreateTexture2D(
				"PageToOffset_Texture",
				256,
				256,
				false,
				TF_Nearest,
				PF_R32_UINT);
		}

		for (const TSharedRef<FVoxelMaterialInstanceRef>& MaterialInstanceRef : MaterialInstanceRefs)
		{
			UMaterialInstanceDynamic* Instance = MaterialInstanceRef->GetInstance();
			if (!ensure(Instance))
			{
				continue;
			}

			Instance->SetTextureParameterValue(STATIC_FNAME("VOXEL_PageToOffset_Texture"), PageToOffset_Texture);

			Impl->MegaMaterialProxy->UpdateInstance(*Instance);
		}
	}

	if (!PageToOffset_Texture)
	{
		ensure(Meshes.Num() == 0);
		return;
	}

	FTextureResource* Resource = PageToOffset_Texture->GetResource();
	if (!ensure(Resource))
	{
		return;
	}

	using FQueuedData = FVoxelNaniteMaterialRendererImpl::FQueuedData;

	const TSharedRef<FQueuedData> QueuedData = MakeSharedCopy(FQueuedData
	{
		Subsystem.GetSubsystem<FVoxelMaterialSubsystem>().GetMaterialInstanceRef(EVoxelMegaMaterialTarget::NaniteMaterialSelection),
		NewLocalToWorld,
		UsedSurfaceTypes,
		MoveTemp(PageToOffset)
	});

	Voxel::RenderTask([Impl = Impl, Resource, QueuedData]
	{
		QueuedData->PageToOffset_Texture = Resource->GetTexture2DRHI();

		Impl->QueuedData_RenderThread = QueuedData;
	});
}