// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLODQuality.h"
#include "VoxelStackLayer.h"
#include "VoxelMesherSettings.h"
#include "VoxelFloatMetadataRef.h"
#include "VoxelComponentSettings.h"

class AVoxelWorld;
class FVoxelMegaMaterialProxy;
struct FVoxelChunkKey;

struct VOXEL_API FVoxelConfig
{
public:
	const TVoxelObjectPtr<UWorld> World;
	const TVoxelObjectPtr<const AVoxelWorld> VoxelWorld;
	const TVoxelObjectPtr<const UObject> VoxelWorldObject;
	const bool bIsEditorWorld;
	const TOptional<FVector> CameraPosition;
	const ERHIFeatureLevel::Type FeatureLevel;

	const FTransform LocalToWorld;
	const FTransform2d LocalToWorld2D;

	const int32 VoxelSize;
	const int32 RenderChunkSize;
	const FVoxelLODQuality LODQuality;
	const double QualityExponent;
	const TSharedRef<FVoxelMegaMaterialProxy> MegaMaterialProxy;
	const FVoxelWeakStackLayer LayerToRender;
	const bool bEnableNanite;
	const int32 MaxLOD;

	const FBodyInstance VisibilityCollision;
	const int32 CollisionChunkSize;
	const FBodyInstance InvokerCollision;

	const bool bEnableNavigation;
	const int32 NavigationChunkSize;
	const int32 MaxAdditionalNavigationChunks;
	const bool bGenerateInsideNavMeshBounds;
	const bool bOnlyGenerateNavigationInEditor;

	const int32 NaniteMaxTessellationLOD;
	const int32 NanitePositionPrecision;

	const bool bEnableLumen;
	const bool bEnableRaytracing;
	const bool bGenerateMeshDistanceFields;
	const TVoxelArray<TVoxelObjectPtr<URuntimeVirtualTexture>> RuntimeVirtualTextures;
	const FVoxelFloatMetadataRef BlockinessMetadata;
	const FVoxelMesherSettings MesherSettings;
	const int32 RaytracingMaxLOD;
	const int32 MeshDistanceFieldMaxLOD;
	const float MeshDistanceFieldBias;
	const FVoxelComponentSettings ComponentSettings;

	explicit FVoxelConfig(const AVoxelWorld& VoxelWorld);

	UE_NONCOPYABLE(FVoxelConfig);

	bool Equals(const FVoxelConfig& Other) const;

	bool CanEnableNanite() const;
	bool CanEnableLumen() const;
	bool UseHardwareRayTracingForLumen() const;

private:
	FVoxelConfig(
		UWorld& World,
		const AVoxelWorld& VoxelWorld);
};