// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Nanite/NaniteShared.h"
#include "Surface/VoxelSurfaceType.h"
#include "MegaMaterial/VoxelRenderMaterial.h"

class FVoxelMegaMaterialProxy;
class FVoxelTextureBufferPool;
class FVoxelNaniteMaterialSelectionPassParameters;

class FVoxelNaniteMaterialRendererImpl : public TSharedFromThis<FVoxelNaniteMaterialRendererImpl>
{
public:
	static TSharedRef<FVoxelNaniteMaterialRendererImpl> Create(const TSharedRef<FVoxelMegaMaterialProxy>& MegaMaterialProxy);

public:
	const TSharedRef<FVoxelMegaMaterialProxy> MegaMaterialProxy;
	const TSharedRef<FVoxelMaterialInstanceRef> DefaultMaterialInstance;
	const TVoxelMap<FVoxelMaterialRenderIndex, TSharedRef<FVoxelMaterialInstanceRef>> MaterialIndexToMaterialInstance;

	TSharedPtr<FVoxelMaterialInstanceRef> GetMaterialInstance(FVoxelMaterialRenderIndex RenderIndex) const;

public:
	struct FQueuedData
	{
		TSharedPtr<FVoxelMaterialRef> MaterialRef;
		FTransform LocalToWorld;
		TVoxelArray<FVoxelSurfaceType> UsedSurfaceTypes;
		TVoxelArray<int32> PageToOffset;
		FTextureRHIRef PageToOffset_Texture;
	};
	TSharedPtr<FQueuedData> QueuedData_RenderThread;

private:
	TSharedPtr<TUniformBuffer<FPrimitiveUniformShaderParameters>> PrimitiveUniformBuffer;

	TSharedPtr<FVoxelMaterialRef> MaterialRef;
	FTransform LocalToWorld;
	TVoxelArray<FVoxelSurfaceType> UsedSurfaceTypes;
	FTextureRHIRef PageToOffset_Texture;

	TRefCountPtr<FRDGPooledBuffer> MaterialIndexToShadingBinExternalBuffer;

	FRDGAsyncScatterUploadBuffer MaterialDepthUploadBuffer;

public:
	void PreRenderBasePass_RenderThread(
		FRDGBuilder& GraphBuilder,
		FSceneView& View);

private:
	FVoxelNaniteMaterialRendererImpl(
		const TSharedRef<FVoxelMegaMaterialProxy>& MegaMaterialProxy,
		const TSharedRef<FVoxelMaterialInstanceRef>& DefaultMaterialInstance,
		TVoxelMap<FVoxelMaterialRenderIndex, TSharedRef<FVoxelMaterialInstanceRef>>&& MaterialIndexToMaterialInstance)
		: MegaMaterialProxy(MegaMaterialProxy)
		, DefaultMaterialInstance(DefaultMaterialInstance)
		, MaterialIndexToMaterialInstance(MoveTemp(MaterialIndexToMaterialInstance))
	{
	}

	void ComputeMaterialSelection(
		const FSceneView& View,
		const FIntPoint& Extent,
		const FVoxelNaniteMaterialSelectionPassParameters& PassParameters,
		FRHICommandList& RHICmdList);
};