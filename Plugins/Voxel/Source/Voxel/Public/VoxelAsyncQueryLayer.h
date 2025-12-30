// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoxelLayersBlueprintLibrary.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "VoxelAsyncQueryLayer.generated.h"

UCLASS(Abstract)
class VOXEL_API UVoxelAsyncQueryVoxelLayerBase : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	//~ Begin UBlueprintAsyncActionBase Interface
	virtual void Activate() override;
	//~ End UBlueprintAsyncActionBase Interface

	virtual void ExecuteCompleted(bool bSuccess, const TArray<FVoxelQueryResult>& Results = {}) {}

protected:
	FVoxelWeakStackLayer WeakLayer;
	TArray<FVector> Positions;
	bool bQuerySurfaceTypes = false;

	UPROPERTY()
	TArray<TObjectPtr<UVoxelFloatMetadata>> LocalMetadatasToQuery;

	int32 LOD = 0;
	TVoxelObjectPtr<UWorld> WeakWorld;

private:
	TMulticastDelegate<void(bool, const TArray<FVoxelQueryResult>&)> OnFinishedReading;
	TOptional<FVoxelFuture> ActiveFuture;
};

UCLASS()
class VOXEL_API UVoxelAsyncQueryVoxelLayer : public UVoxelAsyncQueryVoxelLayerBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, DisplayName = "Async Query Voxel Layer", Category = "Voxel", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "MetadatasToQuery", AdvancedDisplay = "LOD"))
	static UVoxelAsyncQueryVoxelLayer* AsyncQueryVoxelLayer(
		UObject* WorldContextObject,
		FVoxelStackLayer Layer,
		FVector Position,
		bool bQuerySurfaceTypes,
		TArray<UVoxelFloatMetadata*> MetadatasToQuery,
		int32 LOD);

	//~ Begin UVoxelAsyncQueryVoxelLayerBase Interface
	virtual void ExecuteCompleted(bool bSuccess, const TArray<FVoxelQueryResult>& Results) override;
	//~ End UVoxelAsyncQueryVoxelLayerBase Interface

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FVoxelBlueprintAsyncQuery, bool, bSuccess, const FVoxelQueryResult&, Result);

	UPROPERTY(BlueprintAssignable, CallInEditor)
	FVoxelBlueprintAsyncQuery Finish;
};

UCLASS()
class VOXEL_API UVoxelAsyncMultiQueryVoxelLayer : public UVoxelAsyncQueryVoxelLayerBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, DisplayName = "Async Multi Query Voxel Layer", Category = "Voxel", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "MetadatasToQuery", AdvancedDisplay = "LOD"))
	static UVoxelAsyncMultiQueryVoxelLayer* AsyncMultiQueryVoxelLayer(
		UObject* WorldContextObject,
		FVoxelStackLayer Layer,
		TArray<FVector> Positions,
		bool bQuerySurfaceTypes,
		TArray<UVoxelFloatMetadata*> MetadatasToQuery,
		int32 LOD);

	//~ Begin UVoxelAsyncQueryVoxelLayerBase Interface
	virtual void ExecuteCompleted(bool bSuccess, const TArray<FVoxelQueryResult>& Results) override;
	//~ End UVoxelAsyncQueryVoxelLayerBase Interface

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FVoxelBlueprintAsyncMultiQuery, bool, bSuccess, const TArray<FVoxelQueryResult>&, Result);

	UPROPERTY(BlueprintAssignable, CallInEditor)
	FVoxelBlueprintAsyncMultiQuery Finish;
};