// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLODQuality.h"
#include "VoxelMesherSettings.h"
#include "VoxelComponentSettings.h"
#include "VoxelWorld.generated.h"

class FVoxelRuntime;
class UVoxelLayerStack;
class UVoxelMegaMaterial;
class UVoxelFloatMetadata;

extern VOXEL_API bool GVoxelDumpStatus;

DECLARE_DYNAMIC_DELEGATE(FOnVoxelWorldEvent);

UCLASS()
class VOXEL_API AVoxelWorld : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (Units = cm, ClampMin = 1))
	int32 VoxelSize = 100;

	// Higher quality = better looking voxel mesh, but higher memory & rendering cost
	// The voxel mesh will first compute at MinQuality, and slowly increase its resolution up to MaxQuality
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", DisplayName = "LOD Quality")
	FVoxelLODQuality LODQuality;

	// Bias applied to the LOD selection
	// Higher numbers will make far away chunks have a higher resolution
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta=(UIMin = 0.5, UIMax = 1.5))
	double QualityExponent = 1.f;

	// Material used for displacement, lumen scene and fallback
	// Mega materials can render multiple materials at once
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	TObjectPtr<UVoxelMegaMaterial> MegaMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	TObjectPtr<UVoxelLayerStack> LayerStack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bEnableNanite = true;

	// If false, you will need to manually call CreateRuntime
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", AdvancedDisplay)
	bool bCreateRuntimeOnBeginPlay = true;

	// If true, will wait for the voxel world to be generated when entering BeginPlay
	// If you set this to false, you might need to freeze your characters until IsVoxelWorldReady returns true
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", AdvancedDisplay)
	bool bWaitOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", AdvancedDisplay)
	bool bWaitForPCG = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", AdvancedDisplay, meta = (InlineEditConditionToggle))
	bool bLimitMaxLOD = false;

	// If set, chunks with a LOD strictly above this will not be rendered
	// When set you might want to tune your MinQuality to be the same as your MaxQuality, otherwise chunks might be incorrectly skipped
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", AdvancedDisplay, meta = (EditCondition = "bLimitMaxLOD"))
	int32 MaxLOD = 30;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Collision|Visibility")
	FBodyInstance VisibilityCollision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Collision|Invoker", meta = (ClampMin = 1))
	int32 CollisionChunkSize = 32;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Collision|Invoker")
	FBodyInstance InvokerCollision;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Navigation")
	bool bEnableNavigation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Navigation", AdvancedDisplay, meta = (ClampMin = 1))
	int32 NavigationChunkSize = 32;

	// Voxel navigation invokers moving back and forth can lead to a lot of waste as the navmesh gets constantly rebuilt
	// To prevent that, we keep out of range navigation chunks alive
	// This variable control the max number of chunks we can keep alive, in addition to the ones within range of a voxel navigation invoker
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Navigation", AdvancedDisplay)
	int32 MaxAdditionalNavigationChunks = 256;

	// If true, will generate navigation on all voxel chunks inside NavMeshBoundsVolumes
	// Expensive if the bounds are big!
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Navigation")
	bool bGenerateNavigationInsideNavMeshBounds = false;

	// Useful if you plan to bake your navigation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Navigation")
	bool bOnlyGenerateNavigationInEditor = false;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nanite")
	bool bEnableTessellation = true;

	// Chunks above that LOD won't have nanite tessellation
	// Decrease this if you're seeing holes in far chunks
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nanite", AdvancedDisplay)
	int32 NaniteMaxTessellationLOD = 2;

	// Increase this if you're getting holes between far LODs
	// Increasing this will increase Nanite memory usage
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nanite", AdvancedDisplay, meta = (UIMin = 0, UIMax = 8))
	int32 NanitePositionPrecision = 6;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Rendering")
	bool bEnableLumen = false;

	// If true, the voxel world will be visible in ray tracing effects
	// Turning this off will remove it from ray traced reflections, shadows, etc.
	// This will be force-enabled if EnableLumen is true and Lumen is set to use hardware raytracing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Rendering")
	bool bEnableRaytracing = false;

	// If true, the voxel world will generate mesh distance fields
	// This will be force-enabled if EnableLumen is true and Lumen is set to use mesh distance fields
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Rendering")
	bool bGenerateMeshDistanceFields = false;

	// Array of runtime virtual textures into which we draw the mesh for this actor.
	// The material also needs to be set up to output to a virtual texture.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Rendering", meta = (DisplayName = "Draw in Virtual Textures"))
	TArray<TObjectPtr<URuntimeVirtualTexture>> RuntimeVirtualTextures;

	// If set, will be queried to select how blocky a voxel will be rendered
	// If set you should also enable flat normals, otherwise normals might be incorrect
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Rendering")
	TObjectPtr<UVoxelFloatMetadata> BlockinessMetadata;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Rendering")
	FVoxelMesherSettings MesherSettings;

	// Chunks strictly above this LOD won't compute raytracing
	// Increasing this will slightly increase GPU memory usage
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Voxel Rendering")
	int32 RaytracingMaxLOD = 10;

	// Chunks strictly above this LOD won't compute distance fields
	// Increasing this will slow down generation & increase GPU memory usage
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Voxel Rendering")
	int32 MeshDistanceFieldMaxLOD = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Voxel Rendering")
	float MeshDistanceFieldBias = 0.5f;

	// Use this to override specific rendering settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Voxel Rendering")
	FVoxelComponentSettings ComponentSettings;

public:
	AVoxelWorld();
	virtual ~AVoxelWorld() override;

#if WITH_EDITOR
	void SetupForPreview();
#endif

public:
	// If the voxel runtime is not created, no voxel mesh will be rendered
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	bool IsRuntimeCreated() const;

	// Returns true once a voxel world state has been fully computed
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	bool IsVoxelWorldReady() const;

	// If true, the voxel world is processing a new state (eg after a player moved)
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	bool IsProcessingNewState() const;

	// Will return a number between 0 and 1 if IsProcessingNewState is true,
	// and 1 if IsProcessingNewState is false
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	float GetProgress() const;

	// Return the number of tasks currently queued
	// Might go up and down
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	int32 GetNumPendingTasks() const;

public:
	FSimpleMulticastDelegate OnRuntimeCreated;
	FSimpleMulticastDelegate OnRuntimeDestroyed;

	// Delegate will be called when a new state starts computing
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void OnInvalidated(FOnVoxelWorldEvent Delegate);
	void OnInvalidated(FSimpleDelegate Delegate);

	// Delegate will be called when all pending changes are rendered
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void OnNextStateRendered(FOnVoxelWorldEvent Delegate);
	void OnNextStateRendered(FSimpleDelegate Delegate);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void BindOnRuntimeCreated(FOnVoxelWorldEvent Delegate);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void BindOnRuntimeDestroyed(FOnVoxelWorldEvent Delegate);

public:
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void CreateRuntime();

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void DestroyRuntime();

	TSharedPtr<FVoxelRuntime> GetRuntime() const;

public:
	//~ Begin AVoxelActorBase Interface
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void Destroyed() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostLoad() override;
	virtual void Serialize(FArchive& Ar) override;
#if WITH_EDITOR
	virtual bool Modify(bool bAlwaysMarkDirty = true) override;
	virtual void PostEditUndo() override;
	virtual void PreEditChange(FProperty* PropertyThatWillChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool CanBeBaseForCharacter(APawn* Pawn) const override;
#endif
	//~ End AVoxelActorBase Interface

	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

#if WITH_EDITOR
	static void CreateNewIfNeeded_EditorOnly(const UObject* WorldContext);
#endif

private:
	FByteBulkData BulkData;
	bool bDisableModify = false;
	TSharedPtr<FVoxelRuntime> Runtime;

	friend FVoxelRuntime;
};