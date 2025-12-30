// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNavigationInvoker.generated.h"

class UVoxelNavigationInvokerComponent;

class VOXEL_API FVoxelNavigationInvokerView : public TSharedFromThis<FVoxelNavigationInvokerView>
{
public:
	FVoxelNavigationInvokerView(
		int32 ChunkSize,
		const FTransform& LocalToWorld);

	TVoxelFuture<const TVoxelSet<FIntVector>> GetChunks(FVoxelDependencyCollector& DependencyCollector) const;

	void Tick(const TVoxelSet<TObjectPtr<UVoxelNavigationInvokerComponent>>& InvokerComponents);

private:
	const int32 ChunkSize;
	const FTransform LocalToWorld;
	const TSharedRef<FVoxelDependency> Dependency;

	FVoxelFuture Future;
	TVoxelArray<FSphere> LastInvokers;

#if VOXEL_INVALIDATION_TRACKING
	TVoxelArray<TVoxelObjectPtr<const UVoxelNavigationInvokerComponent>> LastInvokerComponents;
#endif

	FVoxelCriticalSection CriticalSection;
	TSharedPtr<const TVoxelSet<FIntVector>> Chunks_RequiresLock;
};

class VOXEL_API FVoxelNavigationInvokerManager : public IVoxelWorldSubsystem
{
public:
	GENERATED_VOXEL_WORLD_SUBSYSTEM_BODY(FVoxelNavigationInvokerManager);

	void LogInvokers();

	TSharedRef<FVoxelNavigationInvokerView> MakeView(
		int32 ChunkSize,
		const FTransform& LocalToWorld);

	//~ Begin IVoxelWorldSubsystem Interface
	virtual void Tick() override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	//~ End IVoxelWorldSubsystem Interface

private:
	double LastTickTime = 0;
	TVoxelSet<TObjectPtr<UVoxelNavigationInvokerComponent>> InvokerComponents;
	TVoxelArray<TWeakPtr<FVoxelNavigationInvokerView>> WeakViews;

	friend UVoxelNavigationInvokerComponent;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UCLASS(ClassGroup = Voxel, HideCategories = ("Rendering", "Physics", "LOD", "Activation", "Navigation", "Cooking", "AssetUserData"), meta = (BlueprintSpawnableComponent))
class VOXEL_API UVoxelNavigationInvokerComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Invoker")
	bool bEnabled = true;

	// In world space, not affected by scale.
	// To avoid constant navmesh recomputes when using Voxel Navmesh Invokers alongside Recast Invokers, the Voxel Navmesh Invoker radius should usually be larger than the radius of the Recast Invokers.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Invoker")
	float Radius = 1000.f;

	UVoxelNavigationInvokerComponent();

	//~ Begin UPrimitiveComponent Interface
	virtual void Serialize(FArchive& Ar) override;
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ End UPrimitiveComponent Interface
};