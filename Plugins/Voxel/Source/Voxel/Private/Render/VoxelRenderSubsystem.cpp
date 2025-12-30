// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Render/VoxelRenderSubsystem.h"
#include "Render/VoxelRenderChunk.h"
#include "Render/VoxelMeshComponent.h"
#include "Nanite/VoxelNaniteComponent.h"
#include "Render/VoxelMeshRenderProxy.h"
#include "Render/VoxelRenderNeighborSubdivider.h"
#include "Nanite/VoxelNaniteMesh.h"
#include "Nanite/VoxelNaniteMaterialRenderer.h"
#include "Collision/VoxelCollider.h"
#include "Collision/VoxelCollisionComponent.h"
#include "Collision/VoxelStaticMeshCollisionComponent.h"
#include "MegaMaterial/VoxelMaterialSubsystem.h"
#include "MegaMaterial/VoxelMegaMaterialProxy.h"
#include "VoxelMesher.h"
#include "VoxelLayers.h"
#include "VoxelRuntime.h"

VOXEL_CONSOLE_VARIABLE(
	VOXEL_API, int32, GVoxelMaxRenderChunks, 2 * 1024 * 1024,
	"voxel.render.MaxRenderChunks",
	"Max number of render chunks allowed before erroring out");

VOXEL_CONSOLE_VARIABLE(
	VOXEL_API, int32, GVoxelMaxChunksToProcessPerPass, 32,
	"voxel.render.MaxChunksToProcessPerPass",
	"Max number of chunks to subdivide per pass when the Quality is good enough but not at max");

VOXEL_CONSOLE_VARIABLE(
	VOXEL_API, float, GVoxelMinQualityDividerWhenEmpty, 4,
	"voxel.render.MinQualityDividerWhenEmpty",
	"Decrease this if some areas do not generate. Decreasing will reduce the amount of empty chunks skipped, reducing gen speed.");

VOXEL_CONSOLE_VARIABLE(
	VOXEL_API, bool, GVoxelShowInvalidatedChunks, false,
	"voxel.render.ShowInvalidatedChunks",
	"If true will show the bounds of invalidated chunks");

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelRenderSubsystem::~FVoxelRenderSubsystem()
{
	if (ChunkKeyToChunk.Num() > 0)
	{
		Voxel::AsyncTask([ChunkKeyToChunk = MakeSharedCopy(MoveTemp(ChunkKeyToChunk))]
		{
			VOXEL_SCOPE_COUNTER_NUM("Destroy FVoxelRenderSubsystem::ChunkKeyToChunk", ChunkKeyToChunk->Num());
			ChunkKeyToChunk->Empty();
		});
	}
}

void FVoxelRenderSubsystem::LoadFromPrevious(FVoxelSubsystem& InPreviousSubsystem)
{
	FVoxelRenderSubsystem& PreviousSubsystem = CastStructChecked<FVoxelRenderSubsystem>(InPreviousSubsystem);

	NaniteMaterialRenderer = PreviousSubsystem.NaniteMaterialRenderer;

	ensure(!PreviousSubsystem.PreviousNaniteMaterialRenderer);
	PreviousNaniteMaterialRenderer = PreviousSubsystem.NaniteMaterialRenderer;
	PreviousSubsystem.NaniteMaterialRenderer = nullptr;

	RootChunks = MoveTemp(PreviousSubsystem.RootChunks);
	ChunkKeyToChunk = MoveTemp(PreviousSubsystem.ChunkKeyToChunk);

	if (PreviousSubsystem.GetConfig().FeatureLevel != GetConfig().FeatureLevel ||

		PreviousSubsystem.GetConfig().VoxelSize != GetConfig().VoxelSize ||
		PreviousSubsystem.GetConfig().RenderChunkSize != GetConfig().RenderChunkSize ||
		PreviousSubsystem.GetConfig().MegaMaterialProxy != GetConfig().MegaMaterialProxy ||
		PreviousSubsystem.GetConfig().LayerToRender != GetConfig().LayerToRender ||
		PreviousSubsystem.GetConfig().bEnableNanite != GetConfig().bEnableNanite ||
		PreviousSubsystem.GetConfig().MaxLOD != GetConfig().MaxLOD ||

		!FVoxelUtilities::BodyInstanceEqual(PreviousSubsystem.GetConfig().VisibilityCollision, GetConfig().VisibilityCollision) ||

		PreviousSubsystem.GetConfig().NaniteMaxTessellationLOD != GetConfig().NaniteMaxTessellationLOD ||
		PreviousSubsystem.GetConfig().NanitePositionPrecision != GetConfig().NanitePositionPrecision ||

		PreviousSubsystem.GetConfig().bEnableLumen != GetConfig().bEnableLumen ||
		PreviousSubsystem.GetConfig().bEnableRaytracing != GetConfig().bEnableRaytracing ||
		PreviousSubsystem.GetConfig().bGenerateMeshDistanceFields != GetConfig().bGenerateMeshDistanceFields ||
		PreviousSubsystem.GetConfig().RuntimeVirtualTextures != GetConfig().RuntimeVirtualTextures ||
		PreviousSubsystem.GetConfig().BlockinessMetadata != GetConfig().BlockinessMetadata ||
		PreviousSubsystem.GetConfig().MesherSettings != GetConfig().MesherSettings ||
		PreviousSubsystem.GetConfig().RaytracingMaxLOD != GetConfig().RaytracingMaxLOD ||
		PreviousSubsystem.GetConfig().MeshDistanceFieldMaxLOD != GetConfig().MeshDistanceFieldMaxLOD ||
		PreviousSubsystem.GetConfig().MeshDistanceFieldBias != GetConfig().MeshDistanceFieldBias ||
		PreviousSubsystem.GetConfig().ComponentSettings != GetConfig().ComponentSettings)
	{
		RootChunks.Reset();

		for (const auto& It : ChunkKeyToChunk)
		{
			DestroyRenderData(It.Value->RenderData);
		}
		ChunkKeyToChunk.Reset_KeepHashSize();

		NaniteMaterialRenderer = nullptr;
	}
}
void FVoxelRenderSubsystem::Initialize()
{
	VOXEL_FUNCTION_COUNTER();

	if (GetConfig().bEnableNanite &&
		!NaniteMaterialRenderer)
	{
		NaniteMaterialRenderer = MakeShared_Stats<FVoxelNaniteMaterialRenderer>(GetConfig().MegaMaterialProxy);
	}
}

void FVoxelRenderSubsystem::AddReferencedObjects(FReferenceCollector& Collector)
{
	VOXEL_FUNCTION_COUNTER();

	Super::AddReferencedObjects(Collector);

	if (NaniteMaterialRenderer)
	{
		NaniteMaterialRenderer->AddReferencedObjects(Collector);
	}
}

void FVoxelRenderSubsystem::Compute()
{
	VOXEL_FUNCTION_COUNTER();

	// This is needed as we could have a dependency tracker invalidated mid-update,
	// which would break subdivision logic
	{
		VOXEL_SCOPE_COUNTER("Copy invalidation");

		for (const auto& It : ChunkKeyToChunk)
		{
			FVoxelRenderChunk& Chunk = *It.Value;

			Chunk.bMeshInvalidated =
				Chunk.MeshDependencyTracker &&
				Chunk.MeshDependencyTracker->IsInvalidated();

			if (GVoxelShowInvalidatedChunks &&
				Chunk.bMeshInvalidated)
			{
				const FVoxelBox Bounds = It.Key.GetBounds(
					GetConfig().RenderChunkSize,
					GetConfig().VoxelSize);

				FVoxelDebugDrawer()
				.LifeTime(0.5f)
				.Color(FLinearColor::Red)
				.Thickness(2.f)
				.DrawBox(Bounds, GetConfig().LocalToWorld);
			}
		}
	}

	if (!TryInitializeRootChunks())
	{
		RootChunks.Empty();

		for (const auto& It : ChunkKeyToChunk)
		{
			DestroyRenderData(It.Value->RenderData);
		}
		ChunkKeyToChunk.Empty();

		if (GetConfig().bEnableNanite)
		{
			FinalizeRender_Nanite();
		}

		return;
	}

	Traverse();
	Collapse();
	Subdivide();
	SubdivideNeighbors();
	FinalizeTraversal();

	StartMeshingTasks().Then_AsyncThread([this]
	{
		StartRenderTasks().Then_AnyThread([this]
		{
			if (GetConfig().bEnableNanite)
			{
				Voxel::AsyncTask([this]
				{
					FinalizeRender_Nanite();
				});
			}
		});
	});
}

void FVoxelRenderSubsystem::Render(FVoxelRuntime& Runtime)
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelConfig& Config = GetConfig();

	if (NaniteMaterialRenderer)
	{
		NaniteMaterialRenderer->UpdateRender(*this, Config.LocalToWorld);
	}

	TVoxelChunkedArray<UVoxelCollisionComponent*> CollisionComponents;
	TVoxelChunkedArray<UVoxelStaticMeshCollisionComponent*> StaticMeshCollisionComponents;
	TVoxelChunkedArray<UVoxelNaniteComponent*> NaniteComponents;
	TVoxelChunkedArray<UVoxelMeshComponent*> MeshComponents;
	{
		VOXEL_SCOPE_COUNTER("Collect components");

		for (const TSharedPtr<FVoxelRenderChunkData>& RenderData : RenderDatasToDestroy)
		{
			if (UVoxelCollisionComponent* Component = RenderData->CollisionComponent.Resolve())
			{
				CollisionComponents.Add(Component);
			}

			if (UVoxelStaticMeshCollisionComponent* Component = RenderData->StaticMeshCollisionComponent.Resolve())
			{
				StaticMeshCollisionComponents.Add(Component);
			}

			if (UVoxelNaniteComponent* Component = RenderData->NaniteComponent.Resolve())
			{
				NaniteComponents.Add(Component);
			}

			if (UVoxelMeshComponent* Component = RenderData->MeshComponent.Resolve())
			{
				MeshComponents.Add(Component);
			}
		}
	}

	{
		VOXEL_SCOPE_COUNTER("RenderDatasToRender");

		UMaterialInterface* NaniteWPOMaterial = GetSubsystem<FVoxelMaterialSubsystem>().GetMaterialInstanceRef(EVoxelMegaMaterialTarget::NaniteWPO)->GetMaterial();
		UMaterialInterface* NaniteDisplacementMaterial = GetSubsystem<FVoxelMaterialSubsystem>().GetMaterialInstanceRef(EVoxelMegaMaterialTarget::NaniteDisplacement)->GetMaterial();
		const TSharedRef<FVoxelMaterialRef> NonNaniteMaterial = GetSubsystem<FVoxelMaterialSubsystem>().GetMaterialInstanceRef(EVoxelMegaMaterialTarget::NonNanite);
		const TSharedRef<FVoxelMaterialRef> LumenMaterial = GetSubsystem<FVoxelMaterialSubsystem>().GetMaterialInstanceRef(EVoxelMegaMaterialTarget::Lumen);

		for (const TSharedPtr<FVoxelRenderChunkData>& RenderData : RenderDatasToRender)
		{
			const FVoxelChunkKey& ChunkKey = RenderData->ChunkKey;

			if (RenderData->Collider)
			{
				UVoxelCollisionComponent* Component = RenderData->CollisionComponent.Resolve();
				if (!Component)
				{
					Component = CollisionComponents.Num() > 0
						? CollisionComponents.Pop()
						: Runtime.NewComponent<UVoxelCollisionComponent>();

					RenderData->CollisionComponent = Component;
				}
				if (!ensure(Component))
				{
					continue;
				}

				Component->SetRelativeScale3D(FVector(1 << ChunkKey.LOD) * Config.VoxelSize);
				Component->SetRelativeLocation(FVector(ChunkKey.ChunkKey) * Config.RenderChunkSize * Config.VoxelSize);
				Component->SetBodyInstance(Config.VisibilityCollision);
				Component->SetCollider(RenderData->Collider);
			}
			else
			{
				if (UVoxelCollisionComponent* Component = RenderData->CollisionComponent.Resolve())
				{
					CollisionComponents.Add(Component);
				}
			}

			if (Config.bIsEditorWorld &&
				RenderData->Collider)
			{
				UVoxelStaticMeshCollisionComponent* Component = RenderData->StaticMeshCollisionComponent.Resolve();
				if (!Component)
				{
					Component = StaticMeshCollisionComponents.Num() > 0
						? StaticMeshCollisionComponents.Pop()
						: Runtime.NewComponent<UVoxelStaticMeshCollisionComponent>();

					RenderData->StaticMeshCollisionComponent = Component;
				}
				if (!ensure(Component))
				{
					continue;
				}

				Component->SetRelativeScale3D(FVector(1 << ChunkKey.LOD) * Config.VoxelSize);
				Component->SetRelativeLocation(FVector(ChunkKey.ChunkKey) * Config.RenderChunkSize * Config.VoxelSize);
				Component->SetBodyInstance(Config.VisibilityCollision);
				Component->SetCollider(RenderData->Collider);

				Component->BodyInstance.SetObjectType(ECC_WorldStatic);
			}
			else
			{
				if (UVoxelStaticMeshCollisionComponent* Component = RenderData->StaticMeshCollisionComponent.Resolve())
				{
					StaticMeshCollisionComponents.Add(Component);
				}
			}

			if (RenderData->NaniteMesh)
			{
				UVoxelNaniteComponent* Component = RenderData->NaniteComponent.Resolve();
				if (!Component)
				{
					Component = NaniteComponents.Num() > 0
						? NaniteComponents.Pop()
						: Runtime.NewComponent<UVoxelNaniteComponent>();

					RenderData->NaniteComponent = Component;
				}
				if (!ensure(Component))
				{
					continue;
				}

				const bool bEnableTessellation = ChunkKey.LOD <= Config.NaniteMaxTessellationLOD;

				Config.ComponentSettings.ApplyToComponent(*Component);

				// If tessellation is disabled, we don't need to render any material as this material is only used during Nanite's vertex stage
				Component->SetNaniteMaterial(bEnableTessellation ? NaniteDisplacementMaterial : NaniteWPOMaterial);

				// If we enable tessellation, we have to scale the mesh vertices to avoid getting a scaled displacement
				Component->SetRelativeScale3D(FVector(
					bEnableTessellation
					? 1.
					: double(1 << ChunkKey.LOD) * Config.VoxelSize));

				Component->SetRelativeLocation(FVector(ChunkKey.ChunkKey) * Config.RenderChunkSize * Config.VoxelSize);

				Component->SetMesh(
					RenderData->NaniteMesh,
					*Config.MegaMaterialProxy,
					NaniteMaterialRenderer);

				FVoxelUtilities::ResetPreviousLocalToWorld(*Component);
			}
			else
			{
				if (UVoxelNaniteComponent* Component = RenderData->NaniteComponent.Resolve())
				{
					NaniteComponents.Add(Component);
				}
			}

			if (RenderData->RenderProxy)
			{
				UVoxelMeshComponent* Component = RenderData->MeshComponent.Resolve();
				if (!Component)
				{
					Component = MeshComponents.Num() > 0
						? MeshComponents.Pop()
						: Runtime.NewComponent<UVoxelMeshComponent>();

					RenderData->MeshComponent = Component;
				}
				if (!ensure(Component))
				{
					continue;
				}

				// Do this now, we need to wait for the buffer pool updates to be done
				Voxel::RenderTask([RenderProxy = RenderData->RenderProxy](FRHICommandListBase& RHICmdList)
				{
					RenderProxy->InitializeVertexFactory_RenderThread(RHICmdList);
				});

				Config.ComponentSettings.ApplyToComponent(*Component);

				RenderData->MeshComponentMaterial = NonNaniteMaterial;

				Component->SetRelativeScale3D(FVector(1 << ChunkKey.LOD) * Config.VoxelSize);
				Component->SetRelativeLocation(FVector(ChunkKey.ChunkKey) * Config.RenderChunkSize * Config.VoxelSize);

				Component->SetRenderProxy(
					RenderData->RenderProxy.ToSharedRef(),
					NonNaniteMaterial,
					LumenMaterial);

				FVoxelUtilities::ResetPreviousLocalToWorld(*Component);
			}
			else
			{
				if (UVoxelMeshComponent* Component = RenderData->MeshComponent.Resolve())
				{
					MeshComponents.Add(Component);
				}
			}
		}
		RenderDatasToRender.Empty();
	}

	{
		VOXEL_SCOPE_COUNTER("Destroy CollisionComponents");

		for (UVoxelCollisionComponent* Component : CollisionComponents)
		{
			Component->SetCollider(nullptr);
		}

		CollisionComponents.ForeachView([&](int32, const TConstVoxelArrayView<UVoxelCollisionComponent*> Components)
		{
			Runtime.RemoveComponents(Components);
		});
	}

	{
		VOXEL_SCOPE_COUNTER("Destroy StaticMeshCollisionComponents");

		for (UVoxelStaticMeshCollisionComponent* Component : StaticMeshCollisionComponents)
		{
			Component->SetCollider(nullptr);
		}

		StaticMeshCollisionComponents.ForeachView([&](int32, const TConstVoxelArrayView<UVoxelStaticMeshCollisionComponent*> Components)
		{
			Runtime.RemoveComponents(Components);
		});
	}

	{
		VOXEL_SCOPE_COUNTER("Destroy NaniteComponents");

		for (UVoxelNaniteComponent* Component : NaniteComponents)
		{
			Component->SetNaniteMaterial(nullptr);

			Component->SetMesh(
				nullptr,
				*Config.MegaMaterialProxy,
				NaniteMaterialRenderer);
		}

		NaniteComponents.ForeachView([&](int32, const TConstVoxelArrayView<UVoxelNaniteComponent*> Components)
		{
			Runtime.RemoveComponents(Components);
		});
	}

	{
		VOXEL_SCOPE_COUNTER("Destroy MeshComponents");

		for (UVoxelMeshComponent* Component : MeshComponents)
		{
			Component->ClearRenderProxy();
		}

		MeshComponents.ForeachView([&](int32, const TConstVoxelArrayView<UVoxelMeshComponent*> Components)
		{
			Runtime.RemoveComponents(Components);
		});
	}

	Voxel::AsyncTask([RenderDatasToDestroyRef = MakeSharedCopy(MoveTemp(RenderDatasToDestroy))]
	{
		VOXEL_SCOPE_COUNTER("Delete RenderDatasToDestroy");

		RenderDatasToDestroyRef->Empty();
	});

	// We're rendered, we can destroy the previous renderer
	PreviousNaniteMaterialRenderer.Reset();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelRenderSubsystem::TryInitializeRootChunks()
{
	VOXEL_FUNCTION_COUNTER();

	if (!GetConfig().CameraPosition.IsSet())
	{
		return false;
	}

	int32 Depth;
	{
		FVoxelDependencyCollector DependencyCollector(STATIC_FNAME("FVoxelRenderSubsystem BoundsToGenerate"));

		BoundsToGenerate = GetLayers().GetBoundsToGenerate(
			GetConfig().LayerToRender,
			DependencyCollector);

		BoundsToGenerateDependencyTracker = Finalize(DependencyCollector);

		if (!BoundsToGenerate.IsValid())
		{
			return false;
		}

		const FVoxelBox ChunkBounds = BoundsToGenerate.GetBox() / GetConfig().VoxelSize / GetConfig().RenderChunkSize;

		double MaxSize = FMath::Max(ChunkBounds.Min.GetAbsMax(), ChunkBounds.Max.GetAbsMax());

		// We need to be able to do FVoxelChunkKey::GetParent on a root chunk for chunk subdivision logic to not be too messy
		// This requires max depth to be 29
		MaxSize = FMath::Min(MaxSize, 1 << 29);

		const FVoxelBox SafeChunkBounds = ChunkBounds.IntersectWith(FVoxelBox(-MaxSize, MaxSize));

		Depth = FMath::CeilLogTwo(FMath::CeilToInt(MaxSize));

		const int32 RootChunkSize = 1 << Depth;
		ensure(FVoxelBox(-RootChunkSize, RootChunkSize).Contains(SafeChunkBounds));
		ensure(!FVoxelBox(-RootChunkSize / 2, RootChunkSize / 2).Contains(SafeChunkBounds));
	}

	if (RootChunks.Num() > 0 &&
		RootChunks[0]->ChunkKey.LOD != Depth)
	{
		RootChunks.Reset();

		for (const auto& It : ChunkKeyToChunk)
		{
			DestroyRenderData(It.Value->RenderData);
		}
		ChunkKeyToChunk.Reset_KeepHashSize();
	}

	const FIntVector Min = FIntVector(-(1 << Depth));
	const FIntVector Max = FIntVector(0);

	const TVoxelStaticArray<FVoxelChunkKey, 8> RootChunkKeys =
	{
		FVoxelChunkKey{ Depth, FIntVector(Min.X, Min.Y, Min.Z) },
		FVoxelChunkKey{ Depth, FIntVector(Max.X, Min.Y, Min.Z) },
		FVoxelChunkKey{ Depth, FIntVector(Min.X, Max.Y, Min.Z) },
		FVoxelChunkKey{ Depth, FIntVector(Max.X, Max.Y, Min.Z) },
		FVoxelChunkKey{ Depth, FIntVector(Min.X, Min.Y, Max.Z) },
		FVoxelChunkKey{ Depth, FIntVector(Max.X, Min.Y, Max.Z) },
		FVoxelChunkKey{ Depth, FIntVector(Min.X, Max.Y, Max.Z) },
		FVoxelChunkKey{ Depth, FIntVector(Max.X, Max.Y, Max.Z) }
	};

	if (RootChunks.Num() == 0)
	{
		for (const FVoxelChunkKey& ChunkKey : RootChunkKeys)
		{
			const TSharedRef<FVoxelRenderChunk> Chunk = MakeShared<FVoxelRenderChunk>(ChunkKey);
			RootChunks.Add(Chunk);
			ChunkKeyToChunk.Add_EnsureNew(ChunkKey, Chunk);
		}
	}
	else
	{
		for (int32 Index = 0; Index < 8; Index++)
		{
			ensure(RootChunks[Index]->ChunkKey == RootChunkKeys[Index]);
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelRenderSubsystem::FScreenSizeHelper::FScreenSizeHelper(const FVoxelSubsystem& Subsystem)
{
	const FVoxelConfig& Config = Subsystem.GetConfig();

	CameraChunkPosition =
		FVector(Config.LocalToWorld.InverseTransformPosition(Config.CameraPosition.GetValue()))
		/ Config.VoxelSize
		/ Config.RenderChunkSize;

	FFloatInterval Quality =
		Config.bIsEditorWorld && !Config.LODQuality.bAlwaysUseGameQuality
		? Config.LODQuality.EditorQuality
		: Config.LODQuality.GameQuality;

	Quality.Min = FMath::Max(Quality.Min, 0);
	Quality.Max = FMath::Max(Quality.Min, Quality.Max);

	MinQuality = Quality.Min;
	MaxQuality = Quality.Max;

	ensure(MinQuality <= MaxQuality);

	ChunkToWorld = Config.RenderChunkSize * Config.VoxelSize;
	QualityExponent = FMath::Clamp(Config.QualityExponent, 0.001f, 100.f);

	MaxChunksToProcessPerPass = GVoxelMaxChunksToProcessPerPass;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelRenderSubsystem::CollapseChunk(FVoxelRenderChunk& Chunk)
{
	ensure(Chunk.Children.Num() == 8);

	for (const TSharedPtr<FVoxelRenderChunk>& Child : Chunk.Children)
	{
		if (Child->Children.Num() > 0)
		{
			CollapseChunk(*Child);
		}
		ensure(Child->Children.Num() == 0);

		ensure(ChunkKeyToChunk.Remove(Child->ChunkKey));
	}

	ensure(Chunk.ChildrenToDestroy.Num() == 0);
	Chunk.ChildrenToDestroy = MoveTemp(Chunk.Children);

	ensure(Chunk.Children.Num() == 0);
}

void FVoxelRenderSubsystem::SubdivideChunk(FVoxelRenderChunk& Chunk)
{
	ensure(Chunk.Children.Num() == 0);

	if (Chunk.ChildrenToDestroy.Num() > 0)
	{
		// Previously collapsed chunk, reuse existing children
		Chunk.Children = MoveTemp(Chunk.ChildrenToDestroy);
	}
	else
	{
		FVoxelUtilities::SetNum(Chunk.Children, 8);

		for (int32 Index = 0; Index < 8; Index++)
		{
			Chunk.Children[Index] = MakeShared<FVoxelRenderChunk>(Chunk.ChunkKey.GetChild(Index));
		}
	}

	ensure(Chunk.Children.Num() == 8);

	for (const TSharedPtr<FVoxelRenderChunk>& Child : Chunk.Children)
	{
		ensure(Child->Children.Num() == 0);
		ChunkKeyToChunk.Add_EnsureNew(Child->ChunkKey, Child);
	}
}

void FVoxelRenderSubsystem::DestroyChunk(FVoxelRenderChunk& Chunk)
{
	ensure(!ChunkKeyToChunk.Contains(Chunk.ChunkKey));
	ensure(Chunk.Children.Num() == 0);

	DestroyRenderData(Chunk.RenderData);

	for (const TSharedPtr<FVoxelRenderChunk>& Child : Chunk.ChildrenToDestroy)
	{
		DestroyChunk(*Child);
	}
}

FVoxelRenderChunk* FVoxelRenderSubsystem::FindNeighbor(
	const FVoxelChunkKey& ChunkKey,
	const int32 DirectionX,
	const int32 DirectionY,
	const int32 DirectionZ) const
{
	checkVoxelSlow(-1 <= DirectionX && DirectionX <= 1);
	checkVoxelSlow(-1 <= DirectionY && DirectionY <= 1);
	checkVoxelSlow(-1 <= DirectionZ && DirectionZ <= 1);

	const int32 MaxLOD = RootChunks[0]->ChunkKey.LOD;

	FVoxelChunkKey Neighbor = ChunkKey;
	{
		const int32 Step = 1 << ChunkKey.LOD;

		Neighbor.ChunkKey.X += DirectionX * Step;
		Neighbor.ChunkKey.Y += DirectionY * Step;
		Neighbor.ChunkKey.Z += DirectionZ * Step;
	}

	FVoxelChunkKey Parent = ChunkKey;

	while (true)
	{
		if (const TSharedPtr<FVoxelRenderChunk>* NeighborChunkPtr = ChunkKeyToChunk.Find(Neighbor))
		{
			FVoxelRenderChunk* NeighborChunk = NeighborChunkPtr->Get();

			if (NeighborChunk->Children.Num() == 0)
			{
				return NeighborChunk;
			}
			else
			{
				// Actual neighbor is one of NeighborChunk children and is higher res than us
				return nullptr;
			}
		}

		if (Neighbor.LOD == MaxLOD)
		{
			// No neighbor or neighbor is higher res than us
			return nullptr;
		}

		checkVoxelSlow(Parent.LOD == Neighbor.LOD);

		if (Parent == Neighbor)
		{
			// No neighbor or neighbor is higher res than us: we walked up the tree until we reached back our own roots
			return nullptr;
		}

		Neighbor = Neighbor.GetParent();
		Parent = Parent.GetParent();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelRenderSubsystem::Traverse()
{
	VOXEL_FUNCTION_COUNTER();

	const FScreenSizeHelper ScreenSizeHelper = FScreenSizeHelper(*this);

	TVoxelArray<FVoxelRenderChunk*> ChunksToTraverse;
	ChunksToTraverse.Reserve(8 * (RootChunks[0]->ChunkKey.LOD + 1));

	for (const TSharedPtr<FVoxelRenderChunk>& RootChunk : RootChunks)
	{
		ChunksToTraverse.Add_EnsureNoGrow(RootChunk.Get());
	}

	while (ChunksToTraverse.Num() > 0)
	{
		FVoxelRenderChunk& Chunk = *ChunksToTraverse.Pop();
		ensure(Chunk.ChildrenToDestroy.Num() == 0);

		if (Chunk.ChunkKey.LOD == 0)
		{
			continue;
		}

		if (ChunkKeyToChunk.Num() > GVoxelMaxRenderChunks)
		{
			VOXEL_MESSAGE(Error, "voxel.MaxRenderChunks reached");
			return;
		}

		const double ChunkQuality = ScreenSizeHelper.GetChunkQuality(Chunk.ChunkKey);

		if (ChunkQuality > ScreenSizeHelper.MaxQuality &&
			Chunk.Children.Num() > 0)
		{
			// Collapse

			CollapseChunk(Chunk);
		}
		else if (
			ChunkQuality < ScreenSizeHelper.MinQuality &&
			Chunk.Children.Num() == 0)
		{
			// Subdivide

			if (Chunk.Mesh &&
				!Chunk.bMeshInvalidated &&
				!Chunk.Mesh->ShouldGenerateChildren(GetConfig().VoxelSize) &&
				ChunkQuality > ScreenSizeHelper.MinQuality / FMath::Max(GVoxelMinQualityDividerWhenEmpty, 1))
			{
				// No need to subdivide
				continue;
			}

			SubdivideChunk(Chunk);
		}

		for (const TSharedPtr<FVoxelRenderChunk>& Child : Chunk.Children)
		{
			ChunksToTraverse.Add_EnsureNoGrow(Child.Get());
		}
	}
}

void FVoxelRenderSubsystem::Collapse()
{
	VOXEL_FUNCTION_COUNTER();

	const FScreenSizeHelper ScreenSizeHelper = FScreenSizeHelper(*this);

	TVoxelArray<FVoxelChunkKey> ChunkKeysToCollapse;
	TVoxelMap<FVoxelChunkKey, int32> ChunkKeyToNumInvalidatedChildren;
	ChunkKeyToNumInvalidatedChildren.Reserve(ChunkKeyToChunk.Num());

	while (true)
	{
		ChunkKeyToNumInvalidatedChildren.Reset_KeepHashSize();

		int32 NumMeshesToCompute = 0;

		for (const auto& It : ChunkKeyToChunk)
		{
			FVoxelRenderChunk& Chunk = *It.Value;
			if (Chunk.Children.Num() > 0)
			{
				continue;
			}

			if (Chunk.Mesh &&
				!Chunk.bMeshInvalidated)
			{
				continue;
			}

			NumMeshesToCompute++;

			FVoxelChunkKey ChunkKey = Chunk.ChunkKey;

			while (true)
			{
				const FVoxelChunkKey ParentChunkKey = ChunkKey.GetParent();
				if (!ChunkKeyToChunk.Contains(ParentChunkKey) ||
					ScreenSizeHelper.GetChunkQuality(ParentChunkKey) < ScreenSizeHelper.MinQuality)
				{
					break;
				}

				ChunkKeyToNumInvalidatedChildren.FindOrAdd(ParentChunkKey)++;

				ChunkKey = ParentChunkKey;
			}
		}

		ChunkKeysToCollapse.Reset(ChunkKeyToNumInvalidatedChildren.Num());

		for (const auto& It : ChunkKeyToNumInvalidatedChildren)
		{
			if (It.Value == 1)
			{
				// Not worth collapsing
				continue;
			}

			ChunkKeysToCollapse.Add(It.Key);
		}

		if (ChunkKeysToCollapse.Num() == 0)
		{
			break;
		}

		{
			VOXEL_SCOPE_COUNTER("Sort");

			ChunkKeysToCollapse.Sort([&](const FVoxelChunkKey& ChunkKeyA, const FVoxelChunkKey& ChunkKeyB)
			{
				return
					ScreenSizeHelper.GetChunkQuality(ChunkKeyA) <
					ScreenSizeHelper.GetChunkQuality(ChunkKeyB);
			});
		}

		while (
			NumMeshesToCompute > ScreenSizeHelper.MaxChunksToProcessPerPass &&
			ChunkKeysToCollapse.Num() > 0)
		{
			const FVoxelChunkKey ChunkKey = ChunkKeysToCollapse.Pop();
			FVoxelRenderChunk& Chunk = *ChunkKeyToChunk[ChunkKey];

			CollapseChunk(Chunk);

			// -1 because we still need to compute the parent
			NumMeshesToCompute -= ChunkKeyToNumInvalidatedChildren[ChunkKey] - 1;
		}

		if (NumMeshesToCompute <= ScreenSizeHelper.MaxChunksToProcessPerPass)
		{
			break;
		}
	}
}

void FVoxelRenderSubsystem::Subdivide()
{
	VOXEL_FUNCTION_COUNTER();

	const FScreenSizeHelper ScreenSizeHelper = FScreenSizeHelper(*this);

	int32 NumMeshesToCompute = 0;

	TVoxelArray<FVoxelChunkKey> ChunkKeysToSubdivide;
	ChunkKeysToSubdivide.Reserve(ChunkKeyToChunk.Num());

	for (const auto& It : ChunkKeyToChunk)
	{
		FVoxelRenderChunk& Chunk = *It.Value;
		if (Chunk.Children.Num() > 0)
		{
			continue;
		}

		if (Chunk.Mesh &&
			!Chunk.bMeshInvalidated)
		{
			if (Chunk.ChunkKey.LOD > 0 &&
				ScreenSizeHelper.GetChunkQuality(Chunk.ChunkKey) <= ScreenSizeHelper.MaxQuality &&
				Chunk.Mesh->ShouldGenerateChildren(GetConfig().VoxelSize))
			{
				ChunkKeysToSubdivide.Add(Chunk.ChunkKey);
			}

			continue;
		}

		NumMeshesToCompute++;
	}

	{
		VOXEL_SCOPE_COUNTER("Sort");

		ChunkKeysToSubdivide.Sort([&](const FVoxelChunkKey& ChunkKeyA, const FVoxelChunkKey& ChunkKeyB)
		{
			return
				ScreenSizeHelper.GetChunkQuality(ChunkKeyA) >
				ScreenSizeHelper.GetChunkQuality(ChunkKeyB);
		});
	}

	while (
		NumMeshesToCompute < ScreenSizeHelper.MaxChunksToProcessPerPass &&
		ChunkKeysToSubdivide.Num() > 0)
	{
		const FVoxelChunkKey ChunkKey = ChunkKeysToSubdivide.Pop();
		FVoxelRenderChunk& Chunk = *ChunkKeyToChunk[ChunkKey];

		SubdivideChunk(Chunk);

		NumMeshesToCompute--;

		for (const TSharedPtr<FVoxelRenderChunk>& Child : Chunk.Children)
		{
			checkVoxelSlow(Child->Children.Num() == 0);

			if (!Child->Mesh ||
				Child->bMeshInvalidated)
			{
				NumMeshesToCompute++;
			}
		}
	}
}

void FVoxelRenderSubsystem::SubdivideNeighbors()
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelRenderNeighborSubdivider Subdivider(*this, RootChunks);
	Subdivider.Traverse();
	Subdivider.ProcessNewChunks();

#if VOXEL_DEBUG
	VOXEL_SCOPE_COUNTER("Checks");

	for (const auto& It : ChunkKeyToChunk)
	{
		if (It.Value->Children.Num() > 0)
		{
			continue;
		}

		for (int32 X = -1; X <= 1; X++)
		{
			for (int32 Y = -1; Y <= 1; Y++)
			{
				for (int32 Z = -1; Z <= 1; Z++)
				{
					const FVoxelRenderChunk* NeighborChunk = FindNeighbor(It.Key, X, Y, Z);
					if (!NeighborChunk)
					{
						continue;
					}

					ensure(NeighborChunk->ChunkKey.LOD - It.Key.LOD <= FVoxelMesher::MaxRelativeLOD);
				}
			}
		}
	}
#endif
}

void FVoxelRenderSubsystem::FinalizeTraversal()
{
	VOXEL_FUNCTION_COUNTER();

	LeafChunkKeys.Reserve(ChunkKeyToChunk.Num());

	for (const auto& It : ChunkKeyToChunk)
	{
		FVoxelRenderChunk& Chunk = *It.Value;

		for (const TSharedPtr<FVoxelRenderChunk>& Child : Chunk.ChildrenToDestroy)
		{
			DestroyChunk(*Child);
		}
		Chunk.ChildrenToDestroy.Empty();

		if (Chunk.Children.Num() == 0)
		{
			LeafChunkKeys.Add(It.Key);
		}
		else
		{
			Chunk.Mesh = {};
			Chunk.MeshDependencyTracker = {};
			DestroyRenderData(Chunk.RenderData);
		}
	}

#if VOXEL_DEBUG
	{
		VOXEL_SCOPE_COUNTER("Checks");

		TVoxelArray<FVoxelRenderChunk*> ChunksToTraverse;
		ChunksToTraverse.Reserve(8 * (RootChunks[0]->ChunkKey.LOD + 1));

		for (const TSharedPtr<FVoxelRenderChunk>& RootChunk : RootChunks)
		{
			ChunksToTraverse.Add_EnsureNoGrow(RootChunk.Get());
		}

		TVoxelSet<FVoxelRenderChunk*> ValidChunks;
		ValidChunks.Reserve(ChunkKeyToChunk.Num());

		while (ChunksToTraverse.Num() > 0)
		{
			FVoxelRenderChunk& Chunk = *ChunksToTraverse.Pop();
			ensure(ChunkKeyToChunk.Contains(Chunk.ChunkKey));
			ensure(Chunk.ChildrenToDestroy.Num() == 0);

			ValidChunks.Add(&Chunk);

			for (const TSharedPtr<FVoxelRenderChunk>& Child : Chunk.Children)
			{
				ChunksToTraverse.Add_EnsureNoGrow(Child.Get());
			}
		}

		for (const auto& It : ChunkKeyToChunk)
		{
			ensure(It.Value->ChunkKey == It.Key);
			ensure(ValidChunks.Contains(It.Value.Get()));
		}

		ensure(ChunkKeyToChunk.Num() == ValidChunks.Num());
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFuture FVoxelRenderSubsystem::StartMeshingTasks()
{
	VOXEL_FUNCTION_COUNTER_NUM(LeafChunkKeys.Num());

	TVoxelMap<FVoxelChunkKey, TVoxelInlineArray<int32, 16>> ChunkKeyXYToChunkKeyZ;
	ChunkKeyXYToChunkKeyZ.Reserve(LeafChunkKeys.Num());

	for (const FVoxelChunkKey& ChunkKey : LeafChunkKeys)
	{
		const TSharedRef<FVoxelRenderChunk> Chunk = ChunkKeyToChunk[ChunkKey].ToSharedRef();
		check(Chunk->Children.Num() == 0);

		if (Chunk->Mesh &&
			!Chunk->bMeshInvalidated)
		{
			continue;
		}

		if (Chunk->ChunkKey.LOD > GetConfig().MaxLOD)
		{
			Chunk->Mesh = MakeShared<FVoxelMesh>(
				Chunk->ChunkKey.LOD,
				FInt64Vector3(Chunk->ChunkKey.ChunkKey) * GetConfig().RenderChunkSize,
				GetConfig().RenderChunkSize,
				FVoxelUtilities::NaNf());

			Chunk->MeshDependencyTracker = {};

			continue;
		}

		DestroyRenderData(Chunk->RenderData);

		FVoxelChunkKey ChunkKeyXY = ChunkKey;
		ChunkKeyXY.ChunkKey.Z = 0;
		ChunkKeyXYToChunkKeyZ.FindOrAdd(ChunkKeyXY).Add(ChunkKey.ChunkKey.Z);
	}

	TVoxelArray<FVoxelFuture> Futures;
	Futures.Reserve(ChunkKeyXYToChunkKeyZ.Num());

	for (auto& It : ChunkKeyXYToChunkKeyZ)
	{
		TVoxelArray<TSharedRef<FVoxelRenderChunk>> Chunks;
		Chunks.Reserve(It.Value.Num());

		// Not needed but makes ShowProcessedChunks nicer
		It.Value.Sort();

		for (const int32 ChunkKeyZ : It.Value)
		{
			FVoxelChunkKey ChunkKey = It.Key;
			ChunkKey.ChunkKey.Z = ChunkKeyZ;

			Chunks.Add_EnsureNoGrow(ChunkKeyToChunk[ChunkKey].ToSharedRef());
		}

		Futures.Add(Voxel::AsyncTask([this, Chunks = MoveTemp(Chunks)]
		{
			const FVoxelConfig& Config = GetConfig();

			TSharedPtr<FVoxelMesherCache> Cache;
			if (Chunks.Num() > 0)
			{
				Cache = MakeShared<FVoxelMesherCache>();
			}

			for (const TSharedRef<FVoxelRenderChunk>& Chunk : Chunks)
			{
				FVoxelDependencyCollector DependencyCollector(STATIC_FNAME("FVoxelRenderChunk"));

				FVoxelMesher Mesher(
					GetLayers(),
					GetSurfaceTypeTable(),
					DependencyCollector,
					Config.LayerToRender,
					Chunk->ChunkKey.LOD,
					FInt64Vector3(Chunk->ChunkKey.ChunkKey) * Config.RenderChunkSize,
					Config.VoxelSize,
					Config.RenderChunkSize,
					Config.LocalToWorld,
					*Config.MegaMaterialProxy,
					Config.BlockinessMetadata,
					Config.MesherSettings,
					Config.bGenerateMeshDistanceFields && Chunk->ChunkKey.LOD <= Config.MeshDistanceFieldMaxLOD);

				Mesher.bRemoveEdges = true;
				Mesher.bQueryMetadata = true;
				Mesher.Cache = Cache;

				Chunk->Mesh = Mesher.CreateMesh();
				Chunk->MeshDependencyTracker = Finalize(DependencyCollector);
			}
		}));
	}

	// If we computed any mesh, schedule another pass to make sure we have no meshes left to subdivide
	// Can't directly use ChunkKeysToSubdivide as we might be waiting on parent chunks to compute for the first time
	bHasChunksToSubdivide = Futures.Num() > 0;

	return FVoxelFuture(Futures);
}

FVoxelFuture FVoxelRenderSubsystem::StartRenderTasks()
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelConfig& Config = GetConfig();

	TVoxelArray<FVoxelFuture> Futures;
	Futures.Reserve(3 * LeafChunkKeys.Num());

	const TSharedRef<FVoxelMaterialInstanceRef> NewMeshComponentMaterial = GetSubsystem<FVoxelMaterialSubsystem>().GetMaterialInstanceRef(EVoxelMegaMaterialTarget::NonNanite);

	for (const FVoxelChunkKey& ChunkKey : LeafChunkKeys)
	{
		const TSharedRef<FVoxelRenderChunk> Chunk = ChunkKeyToChunk[ChunkKey].ToSharedRef();
		check(Chunk->Children.Num() == 0);
		check(Chunk->Mesh);

		if (Chunk->Mesh->IsEmpty())
		{
			DestroyRenderData(Chunk->RenderData);
			continue;
		}

		FVoxelChunkNeighborInfo NeighborInfo;
		for (int32 X = -1; X <= 1; X++)
		{
			for (int32 Y = -1; Y <= 1; Y++)
			{
				for (int32 Z = -1; Z <= 1; Z++)
				{
					const FVoxelRenderChunk* NeighborChunk = FindNeighbor(ChunkKey, X, Y, Z);
					if (!NeighborChunk ||
						NeighborChunk->ChunkKey.LOD > Config.MaxLOD)
					{
						continue;
					}

					ensureVoxelSlow(NeighborChunk->ChunkKey.LOD - ChunkKey.LOD <= FVoxelMesher::MaxRelativeLOD);
					NeighborInfo.SetLOD(X, Y, Z, NeighborChunk->ChunkKey.LOD);
				}
			}
		}

		TSharedPtr<FVoxelRenderChunkData>& RenderData = Chunk->RenderData;

		if (RenderData &&
			RenderData->NeighborInfo == NeighborInfo)
		{
			if (RenderData->RenderProxy &&
				RenderData->MeshComponentMaterial != NewMeshComponentMaterial)
			{
				// Don't invalidate, but still re-render the chunk to update its material
				RenderDatasToRender.Add(RenderData);
			}

			continue;
		}

		DestroyRenderData(RenderData);

		RenderData = MakeShared<FVoxelRenderChunkData>(Chunk->ChunkKey, NeighborInfo);
		RenderDatasToRender.Add(RenderData);

		if (Config.VisibilityCollision.GetCollisionEnabled() != ECollisionEnabled::NoCollision)
		{
			Futures.Add_EnsureNoGrow(Voxel::AsyncTask([=]
			{
				RenderData->Collider = FVoxelCollider::Create(*Chunk->Mesh);
			}));
		}

		if (Config.bEnableNanite)
		{
			Futures.Add_EnsureNoGrow(Voxel::AsyncTask([=, this]
			{
				const TVoxelFuture<TSharedPtr<FVoxelNaniteMesh>> FutureNaniteMesh = FVoxelNaniteMesh::Create(
					*this,
					Chunk->Mesh.ToSharedRef(),
					NeighborInfo);

				return FutureNaniteMesh.Then_AnyThread([=, this](const TSharedPtr<FVoxelNaniteMesh>& NewNaniteMesh)
				{
					ensure(NewNaniteMesh);
					RenderData->NaniteMesh = NewNaniteMesh;
				});
			}));
		}

		const bool bRenderInBasePass = !Config.bEnableNanite;
		const bool bEnableRaytracing = Config.bEnableRaytracing && ChunkKey.LOD <= Config.RaytracingMaxLOD;
		const bool bGenerateMeshDistanceField = Config.bGenerateMeshDistanceFields && ChunkKey.LOD <= Config.MeshDistanceFieldMaxLOD;

		if (bRenderInBasePass ||
			bEnableRaytracing ||
			bGenerateMeshDistanceField ||
			Config.RuntimeVirtualTextures.Num() > 0)
		{
			Futures.Add_EnsureNoGrow(Voxel::AsyncTask([=, this]
			{
				const TSharedRef<FVoxelMeshRenderProxy> RenderProxy = Voxel::MakeShareable_RenderThread(new FVoxelMeshRenderProxy(
					Chunk->Mesh.ToSharedRef(),
					bRenderInBasePass,
					bEnableRaytracing,
					bGenerateMeshDistanceField,
					GetConfig().RuntimeVirtualTextures,
					NeighborInfo));

				return RenderProxy->Initialize_AsyncThread(*this).Then_AnyThread([=, this]
				{
					RenderData->RenderProxy = RenderProxy;
				});
			}));
		}
	}

	return FVoxelFuture(Futures);
}

void FVoxelRenderSubsystem::FinalizeRender_Nanite()
{
	VOXEL_FUNCTION_COUNTER();
	check(GetConfig().bEnableNanite);

	TVoxelSet<TSharedPtr<const FVoxelNaniteMesh>> NaniteMeshes;
	NaniteMeshes.Reserve(LeafChunkKeys.Num());

	for (const FVoxelChunkKey& ChunkKey : LeafChunkKeys)
	{
		const TSharedRef<FVoxelRenderChunk> Chunk = ChunkKeyToChunk[ChunkKey].ToSharedRef();
		check(Chunk->Children.Num() == 0);
		check(Chunk->Mesh);

		if (!Chunk->RenderData ||
			!Chunk->RenderData->NaniteMesh)
		{
			continue;
		}

		NaniteMeshes.Add_EnsureNew(Chunk->RenderData->NaniteMesh);
	}

	NaniteMaterialRenderer->PrepareRender(MoveTemp(NaniteMeshes));
}