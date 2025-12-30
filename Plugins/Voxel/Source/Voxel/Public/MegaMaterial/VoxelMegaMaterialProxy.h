// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMetadataRef.h"
#include "VoxelRenderMaterial.h"
#include "Surface/VoxelSurfaceType.h"
#include "VoxelMegaMaterialTarget.h"

class AVoxelWorld;
class UVoxelMegaMaterial;
class FVoxelTextureBufferPool;
class FVoxelMegaMaterialUsageTracker;
class UVoxelMegaMaterialGeneratedData;
struct FVoxelSurfaceTypeBlend;

class VOXEL_API FVoxelMegaMaterialProxy
{
public:
	const TVoxelObjectPtr<UVoxelMegaMaterial> WeakMegaMaterial;
	const bool bDetectNewSurfaces;

	static TSharedRef<FVoxelMegaMaterialProxy> Default();

	UE_NONCOPYABLE(FVoxelMegaMaterialProxy);

public:
	int32 NumMetadatas() const
	{
		return MetadataIndexToMetadata.Num();
	}
	const TVoxelMap<FVoxelMaterialRenderIndex, TVoxelObjectPtr<UMaterialInterface>>& GetMaterialIndexToMaterial() const
	{
		return RenderIndexToMaterial;
	}
	TConstVoxelArrayView<FVoxelMetadataRef> GetMetadataIndexToMetadata() const
	{
		return MetadataIndexToMetadata;
	}
	TVoxelObjectPtr<UMaterialInterface> GetTargetMaterial(const EVoxelMegaMaterialTarget Target) const
	{
		return TargetToMaterial.FindRef(Target);
	}
	FVoxelTextureBufferPool& GetAttributeIndicesBufferPool() const
	{
		return *AttributeIndicesBufferPool;
	}
	FVoxelTextureBufferPool& GetMaterialBufferPool() const
	{
		return *MaterialBufferPool;
	}
	TSharedPtr<FVoxelTextureBufferPool> GetMetadataBufferPool(const FVoxelMetadataRef Metadata) const
	{
		return MetadataToBufferPool.FindRef(Metadata);
	}
	const TVoxelMap<FVoxelMetadataRef, TSharedPtr<FVoxelTextureBufferPool>>& GetMetadataToBufferPool() const
	{
		return MetadataToBufferPool;
	}

public:
	void UpdateInstance(UMaterialInstanceDynamic& Instance);
	FVoxelMaterialRenderIndex GetRenderIndex(FVoxelSurfaceType SurfaceType) const;
	TConstVoxelArrayView<FVoxelMetadataRef> GetUsedMetadatas(FVoxelMaterialRenderIndex RenderIndex) const;
	TVoxelArray<FVoxelRenderMaterial> GetRenderMaterials(TConstVoxelArrayView<FVoxelSurfaceTypeBlend> SurfaceTypes) const;

public:
#if WITH_EDITOR
	void LogErrors(
		ERHIFeatureLevel::Type FeatureLevel,
		const TSet<EVoxelMegaMaterialTarget>& Targets,
		AVoxelWorld* VoxelWorld) const;
#endif

private:
	TVoxelMap<FVoxelMaterialRenderIndex, TVoxelObjectPtr<UMaterialInterface>> RenderIndexToMaterial;
	TVoxelMap<FVoxelMaterialRenderIndex, TVoxelArray<FVoxelMetadataRef>> RenderIndexToUsedMetadatas;
	TVoxelMap<FVoxelSurfaceType, FVoxelMaterialRenderIndex> SurfaceTypeToRenderIndex;

	TVoxelArray<FVoxelMetadataRef> MetadataIndexToMetadata;
	TVoxelMap<EVoxelMegaMaterialTarget, TVoxelObjectPtr<UMaterialInterface>> TargetToMaterial;

	TSharedPtr<FVoxelTextureBufferPool> AttributeIndicesBufferPool;
	TSharedPtr<FVoxelTextureBufferPool> MaterialBufferPool;
	TVoxelMap<FVoxelMetadataRef, TSharedPtr<FVoxelTextureBufferPool>> MetadataToBufferPool;

#if WITH_EDITOR
	const TSharedRef<FVoxelMegaMaterialUsageTracker> UsageTracker;
#endif

	explicit FVoxelMegaMaterialProxy(UVoxelMegaMaterial& MegaMaterial);

	void Initialize(const FVoxelMegaMaterialProxy* OldProxy);
	bool Equals(const FVoxelMegaMaterialProxy& Other) const;

	friend UVoxelMegaMaterial;
	friend UVoxelMegaMaterialGeneratedData;
};