// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelStackLayer.h"
#include "VoxelFloatMetadataRef.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoxelLayersBlueprintLibrary.generated.h"

class FVoxelLayers;
class FVoxelSurfaceTypeTable;
class UVoxelSurfaceTypeInterface;
struct FVoxelFloatMetadataRef;

USTRUCT(BlueprintType)
struct FVoxelQueryResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float Value = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	TObjectPtr<UVoxelSurfaceTypeInterface> SurfaceType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	TMap<TObjectPtr<UVoxelFloatMetadata>, float> Metadata;
};

UCLASS()
class VOXEL_API UVoxelLayersBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Voxel", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "MetadatasToQuery", AdvancedDisplay = "LOD", bQuerySurfaceTypes = true))
	static bool QueryVoxelLayer(
		UObject* WorldContextObject,
		FVoxelStackLayer Layer,
		FVector Position,
		bool bQuerySurfaceTypes,
		TArray<UVoxelFloatMetadata*> MetadatasToQuery,
		int32 LOD,
		FVoxelQueryResult& Result);

	UFUNCTION(BlueprintCallable, Category = "Voxel", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "MetadatasToQuery", AdvancedDisplay = "LOD", bQuerySurfaceTypes = true))
	static bool MultiQueryVoxelLayer(
		UObject* WorldContextObject,
		FVoxelStackLayer Layer,
		TArray<FVector> Positions,
		bool bQuerySurfaceTypes,
		TArray<UVoxelFloatMetadata*> MetadatasToQuery,
		int32 LOD,
		TArray<FVoxelQueryResult>& Result);

public:
	static TArray<FVoxelQueryResult> MultiQueryVoxelLayer_AnyThread(
		const FVoxelLayers& Layers,
		const FVoxelSurfaceTypeTable& SurfaceTypeTable,
		FVoxelDependencyCollector& DependencyCollector,
		const TArray<FVector>& Positions,
		const FVoxelWeakStackLayer& WeakLayer,
		bool bQuerySurfaceTypes,
		TConstVoxelArrayView<FVoxelFloatMetadataRef> MetadatasToQuery,
		int32 LOD);
};