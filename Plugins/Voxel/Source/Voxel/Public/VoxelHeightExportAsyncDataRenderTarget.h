// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelStackLayer.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "VoxelHeightExportAsyncDataRenderTarget.generated.h"

class FVoxelLayers;
class FVoxelSurfaceTypeTable;
class UTextureRenderTarget2D;
struct FVoxelHeightExportChannelData;

UCLASS()
class VOXEL_API UVoxelHeightExportAsyncDataRenderTarget : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, DisplayName = "Async Export Data to Render Target", Category = "Voxel", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", AdvancedDisplay = "LOD", Layer = "(Type=Height,StackName=\"Default\",LayerName=\"Default\")"))
	static UVoxelHeightExportAsyncDataRenderTarget* AsyncExportDataToRenderTarget(
		UObject* WorldContextObject,
		const FBox2D& Bounds,
		UTextureRenderTarget2D* RenderTarget,
		FVoxelStackHeightLayer Layer,
		FVoxelHeightExportChannelData R,
		FVoxelHeightExportChannelData G,
		FVoxelHeightExportChannelData B,
		FVoxelHeightExportChannelData A,
		int32 LOD);

public:
	//~ Begin UBlueprintAsyncActionBase Interface
	virtual void Activate() override;
	//~ End UBlueprintAsyncActionBase Interface

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVoxelBlueprintAsync, bool, bSuccess);

	UPROPERTY(BlueprintAssignable, CallInEditor)
	FVoxelBlueprintAsync Finish;

private:
	void ReadData(
		const TSharedRef<FVoxelLayers>& Layers,
		const TSharedRef<FVoxelSurfaceTypeTable>& SurfaceTypeTable);

	void FinishedReading(bool bSuccess);
	void ExecuteCompleted(bool bSuccess);

private:
	FBox2D Bounds;
	TArray<FVoxelHeightExportChannelData> Channels;
	FVoxelWeakStackLayer WeakLayer;
	int32 LOD;
	TVoxelObjectPtr<UWorld> WeakWorld;

	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;

	TSharedPtr<TVoxelArray<uint8>> Values;
	TMulticastDelegate<void(bool)> OnFinishedReading;

	TOptional<FVoxelFuture> ActiveFuture;
};