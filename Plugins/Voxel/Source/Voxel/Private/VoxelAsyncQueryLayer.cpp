// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelAsyncQueryLayer.h"
#include "VoxelLayers.h"
#include "Surface/VoxelSurfaceTypeTable.h"

void UVoxelAsyncQueryVoxelLayerBase::Activate()
{
	Super::Activate();

	if (Positions.Num() == 0)
	{
		ExecuteCompleted(false);
		return;
	}

	UWorld* World = WeakWorld.Resolve();
	if (!World)
	{
		VOXEL_MESSAGE(Error, "World is null");
		ExecuteCompleted(false);
		return;
	}

	if (ActiveFuture.IsSet())
	{
		Voxel::FlushGameTasks();
		return;
	}

	const TSharedRef<FVoxelLayers> Layers = FVoxelLayers::Get(World);
	const TSharedRef<FVoxelSurfaceTypeTable> SurfaceTypeTable = FVoxelSurfaceTypeTable::Get();

	OnFinishedReading.AddLambda(MakeWeakObjectPtrLambda(this, [this](const bool bSuccess, const TArray<FVoxelQueryResult>& Results)
	{
		ExecuteCompleted(bSuccess, Results);
	}));

	ActiveFuture = Voxel::AsyncTask([
		Layers,
		SurfaceTypeTable,
		Positions = Positions,
		WeakLayer = WeakLayer,
		bQuerySurfaceTypes = bQuerySurfaceTypes,
		MetadatasToQuery = FVoxelFloatMetadataRef::GetUniqueValidRefs(LocalMetadatasToQuery),
		LOD = LOD,
		OnFinishedReading = OnFinishedReading]
	{
		const TArray<FVoxelQueryResult> Result = UVoxelLayersBlueprintLibrary::MultiQueryVoxelLayer_AnyThread(
			*Layers,
			*SurfaceTypeTable,
			FVoxelDependencyCollector::Null,
			Positions,
			WeakLayer,
			bQuerySurfaceTypes,
			MetadatasToQuery,
			LOD);

		return Voxel::GameTask([OnFinishedReading, Result]
		{
			VOXEL_FUNCTION_COUNTER();
			check(IsInGameThread());

			OnFinishedReading.Broadcast(Result.Num() > 0, Result);
		});
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UVoxelAsyncQueryVoxelLayer* UVoxelAsyncQueryVoxelLayer::AsyncQueryVoxelLayer(
	UObject* WorldContextObject,
	const FVoxelStackLayer Layer,
	const FVector Position,
	const bool bQuerySurfaceTypes,
	const TArray<UVoxelFloatMetadata*> MetadatasToQuery,
	const int32 LOD)
{
	UVoxelAsyncQueryVoxelLayer* Action = NewObject<UVoxelAsyncQueryVoxelLayer>();
	Action->WeakLayer = FVoxelWeakStackLayer(Layer);
	Action->Positions = { Position };
	Action->bQuerySurfaceTypes = bQuerySurfaceTypes;
	Action->LocalMetadatasToQuery = MetadatasToQuery;
	Action->LOD = LOD;
	Action->WeakWorld = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UVoxelAsyncQueryVoxelLayer::ExecuteCompleted(const bool bSuccess, const TArray<FVoxelQueryResult>& Results)
{
	VOXEL_FUNCTION_COUNTER();
	if (!ensure(IsInGameThread()))
	{
		return;
	}

	ensure(!bSuccess || Results.Num() == 1);

	FEditorScriptExecutionGuard AllowScripts;
	Finish.Broadcast(bSuccess, Results.Num() > 0 ? Results[0] : FVoxelQueryResult());
	SetReadyToDestroy();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UVoxelAsyncMultiQueryVoxelLayer* UVoxelAsyncMultiQueryVoxelLayer::AsyncMultiQueryVoxelLayer(
	UObject* WorldContextObject,
	const FVoxelStackLayer Layer,
	const TArray<FVector> Positions,
	const bool bQuerySurfaceTypes,
	const TArray<UVoxelFloatMetadata*> MetadatasToQuery,
	const int32 LOD)
{
	UVoxelAsyncMultiQueryVoxelLayer* Action = NewObject<UVoxelAsyncMultiQueryVoxelLayer>();
	Action->WeakLayer = FVoxelWeakStackLayer(Layer);
	Action->Positions = Positions;
	Action->bQuerySurfaceTypes = bQuerySurfaceTypes;
	Action->LocalMetadatasToQuery = MetadatasToQuery;
	Action->LOD = LOD;
	Action->WeakWorld = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UVoxelAsyncMultiQueryVoxelLayer::ExecuteCompleted(const bool bSuccess, const TArray<FVoxelQueryResult>& Results)
{
	VOXEL_FUNCTION_COUNTER();
	if (!ensure(IsInGameThread()))
	{
		return;
	}

	ensure(!bSuccess || Results.Num() > 0);

	FEditorScriptExecutionGuard AllowScripts;
	Finish.Broadcast(bSuccess, Results);
	SetReadyToDestroy();
}