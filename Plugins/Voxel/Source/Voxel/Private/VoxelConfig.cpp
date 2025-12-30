// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelConfig.h"
#include "VoxelWorld.h"
#include "Engine/RendererSettings.h"
#include "MegaMaterial/VoxelMaterialHook.h"
#include "MegaMaterial/VoxelMegaMaterial.h"
#include "MegaMaterial/VoxelMegaMaterialProxy.h"
#include "Collision/VoxelCollisionChannels.h"
#include "VT/RuntimeVirtualTexture.h"

extern int32 GVoxelMegaMaterialDebugMode;

FVoxelConfig::FVoxelConfig(const AVoxelWorld& VoxelWorld)
	: FVoxelConfig(*VoxelWorld.GetWorld(), VoxelWorld)
{
}

FVoxelConfig::FVoxelConfig(
	UWorld& World,
	const AVoxelWorld& VoxelWorld)
	: World(&World)
	, VoxelWorld(&VoxelWorld)
	, VoxelWorldObject(&VoxelWorld)
	, bIsEditorWorld(!World.IsGameWorld())
	, CameraPosition(INLINE_LAMBDA
	{
		TOptional<FVector> Position = FVoxelUtilities::GetCameraPosition(&World);
		if (Position)
		{
			// Round to voxel size to reduce frequency of invalidations
			Position = FVoxelUtilities::RoundToFloat(*Position / VoxelWorld.VoxelSize) * VoxelWorld.VoxelSize;
		}
		return Position;
	})
	, FeatureLevel(World.GetFeatureLevel())

	, LocalToWorld(VoxelWorld.ActorToWorld())
	, LocalToWorld2D(FVoxelUtilities::MakeTransform2(LocalToWorld))

	, VoxelSize(FMath::Max(VoxelWorld.VoxelSize, 1))
	, RenderChunkSize(32)
	, LODQuality(VoxelWorld.LODQuality)
	, QualityExponent(VoxelWorld.QualityExponent)
	, MegaMaterialProxy(INLINE_LAMBDA
	{
		UVoxelMegaMaterial* MegaMaterial = VoxelWorld.MegaMaterial;
		if (!MegaMaterial)
		{
			return FVoxelMegaMaterialProxy::Default();
		}
		return MegaMaterial->GetProxy();
	})
	, LayerToRender(FVoxelStackLayer(VoxelWorld.LayerStack, nullptr))
	, bEnableNanite(
		VoxelWorld.bEnableNanite &&
		CanEnableNanite() &&
		GVoxelMegaMaterialDebugMode == 0)
	, MaxLOD(VoxelWorld.bLimitMaxLOD ? VoxelWorld.MaxLOD : MAX_int32)

	, VisibilityCollision(VoxelWorld.VisibilityCollision)
	, CollisionChunkSize(FMath::Clamp(VoxelWorld.CollisionChunkSize, 1, 64))
	, InvokerCollision(VoxelWorld.InvokerCollision)

	, bEnableNavigation(VoxelWorld.bEnableNavigation)
	, NavigationChunkSize(FMath::Clamp(VoxelWorld.NavigationChunkSize, 1, 64))
	, MaxAdditionalNavigationChunks(VoxelWorld.MaxAdditionalNavigationChunks)
	, bGenerateInsideNavMeshBounds(VoxelWorld.bGenerateNavigationInsideNavMeshBounds)
	, bOnlyGenerateNavigationInEditor(VoxelWorld.bOnlyGenerateNavigationInEditor)

	, NaniteMaxTessellationLOD(VoxelWorld.bEnableTessellation ? VoxelWorld.NaniteMaxTessellationLOD : -1)
	, NanitePositionPrecision(VoxelWorld.NanitePositionPrecision)

	, bEnableLumen(VoxelWorld.bEnableLumen && CanEnableLumen())
	, bEnableRaytracing(VoxelWorld.bEnableRaytracing || (bEnableLumen && UseHardwareRayTracingForLumen()))
	, bGenerateMeshDistanceFields(VoxelWorld.bGenerateMeshDistanceFields || (bEnableLumen && !UseHardwareRayTracingForLumen()))
	, RuntimeVirtualTextures(VoxelWorld.RuntimeVirtualTextures)
	, BlockinessMetadata(VoxelWorld.BlockinessMetadata)
	, MesherSettings(VoxelWorld.MesherSettings)
	, RaytracingMaxLOD(VoxelWorld.RaytracingMaxLOD)
	, MeshDistanceFieldMaxLOD(VoxelWorld.MeshDistanceFieldMaxLOD)
	, MeshDistanceFieldBias(VoxelWorld.MeshDistanceFieldBias)
	, ComponentSettings(VoxelWorld.ComponentSettings)
{
	if (!VoxelWorld.ActorToWorld().GetScale3D().Equals(FVector::OneVector))
	{
		VOXEL_MESSAGE(Error, "{0}: VoxelWorld should have a uniform scale of 1", VoxelWorld);
	}

#if WITH_EDITOR
	FVoxelMaterialHook::EnsureIsEnabled();
#endif

	if (World.WorldType == EWorldType::Editor ||
		World.WorldType == EWorldType::EditorPreview)
	{
		ConstCast(VisibilityCollision).SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		// Needed for foliage painting
		ConstCast(VisibilityCollision).SetResponseToChannel(ECC_WorldStatic, ECR_Block);

		ConstCast(VisibilityCollision).SetResponseToChannel(ECC_VoxelEditor, ECR_Block);

		ConstCast(VisibilityCollision).SetObjectType(ECC_VoxelEditor);
	}
}

bool FVoxelConfig::Equals(const FVoxelConfig& Other) const
{
	VOXEL_FUNCTION_COUNTER();

	if (CameraPosition.IsSet() != Other.CameraPosition.IsSet())
	{
		return false;
	}

	if (CameraPosition &&
		!CameraPosition->Equals(Other.CameraPosition.GetValue()))
	{
		return false;
	}

	if (FeatureLevel != Other.FeatureLevel)
	{
		return false;
	}

	if (!LocalToWorld.Equals(Other.LocalToWorld))
	{
		return false;
	}

	if (VoxelSize != Other.VoxelSize ||
		RenderChunkSize != Other.RenderChunkSize ||
		LODQuality != Other.LODQuality ||
		QualityExponent != Other.QualityExponent ||
		MegaMaterialProxy != Other.MegaMaterialProxy ||
		LayerToRender != Other.LayerToRender ||
		bEnableNanite != Other.bEnableNanite ||
		MaxLOD != Other.MaxLOD)
	{
		return false;
	}

	if (!FVoxelUtilities::BodyInstanceEqual(VisibilityCollision, Other.VisibilityCollision) ||
		CollisionChunkSize != Other.CollisionChunkSize ||
		!FVoxelUtilities::BodyInstanceEqual(InvokerCollision, Other.InvokerCollision))
	{
		return false;
	}

	if (bEnableNavigation != Other.bEnableNavigation ||
		NavigationChunkSize != Other.NavigationChunkSize ||
		MaxAdditionalNavigationChunks != Other.MaxAdditionalNavigationChunks ||
		bGenerateInsideNavMeshBounds != Other.bGenerateInsideNavMeshBounds ||
		bOnlyGenerateNavigationInEditor != Other.bOnlyGenerateNavigationInEditor)
	{
		return false;
	}

	if (NaniteMaxTessellationLOD != Other.NaniteMaxTessellationLOD ||
		NanitePositionPrecision != Other.NanitePositionPrecision)
	{
		return false;
	}

	if (bEnableLumen != Other.bEnableLumen ||
		bEnableRaytracing != Other.bEnableRaytracing ||
		bGenerateMeshDistanceFields != Other.bGenerateMeshDistanceFields ||
		RuntimeVirtualTextures != Other.RuntimeVirtualTextures ||
		BlockinessMetadata != Other.BlockinessMetadata ||
		MesherSettings != Other.MesherSettings ||
		RaytracingMaxLOD != Other.RaytracingMaxLOD ||
		MeshDistanceFieldMaxLOD != Other.MeshDistanceFieldMaxLOD ||
		MeshDistanceFieldBias != Other.MeshDistanceFieldBias ||
		ComponentSettings != Other.ComponentSettings)
	{
		return false;
	}

	if (ComponentSettings != Other.ComponentSettings ||
		BlockinessMetadata != Other.BlockinessMetadata ||
		MesherSettings != Other.MesherSettings)
	{
		return false;
	}

	return true;
}

bool FVoxelConfig::CanEnableNanite() const
{
	return UseNanite(GShaderPlatformForFeatureLevel[FeatureLevel]);
}

bool FVoxelConfig::CanEnableLumen() const
{
	return
		DoesPlatformSupportLumenGI(GShaderPlatformForFeatureLevel[FeatureLevel]) &&
		GetDefault<URendererSettings>()->DynamicGlobalIllumination == EDynamicGlobalIlluminationMethod::Lumen;
}

bool FVoxelConfig::UseHardwareRayTracingForLumen() const
{
	// See Lumen::UseHardwareRayTracing

	static const auto CVarLumenUseHardwareRayTracing = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.HardwareRayTracing"));
	if (!ensure(CVarLumenUseHardwareRayTracing))
	{
		return false;
	}

	return
		IsRayTracingEnabled(GShaderPlatformForFeatureLevel[FeatureLevel])
		&& (GRHISupportsInlineRayTracing || (GRHISupportsRayTracingShaders && GRHISupportsRayTracingDispatchIndirect))
		&& CVarLumenUseHardwareRayTracing->GetInt() != 0;
}