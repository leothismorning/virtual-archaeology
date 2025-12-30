// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelStackLayer.h"
#include "VoxelFloatMetadataRef.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoxelHeightGraphBlueprintLibrary.generated.h"

class FVoxelLayers;
class FVoxelSurfaceTypeTable;
class UTextureRenderTarget2D;

UENUM(BlueprintType)
enum class EVoxelHeightExportChannelType : uint8
{
	None,
	Height,
	Metadata
};

USTRUCT(BlueprintType)
struct VOXEL_API FVoxelHeightExportChannelData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	EVoxelHeightExportChannelType ChannelType = EVoxelHeightExportChannelType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel", meta = (EditCondition = "ChannelType == EVoxelHeightExportChannelType::Metadata"))
	TObjectPtr<UVoxelFloatMetadata> Metadata;

	FVoxelFloatMetadataRef MetadataRef;
};

UCLASS()
class VOXEL_API UVoxelHeightGraphBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Voxel", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "LOD", Layer = "(Type=Height,StackName=\"Default\",LayerName=\"Default\")"))
	static bool SyncExportDataToRenderTarget(
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
	static bool IsValidDataForExport(
		const UWorld* World,
		const UTextureRenderTarget2D* RenderTarget,
		const TArray<FVoxelHeightExportChannelData>& Channels);

	static void ReadValues(
		const FVoxelLayers& Layers,
		const FVoxelSurfaceTypeTable& SurfaceTypeTable,
		FVoxelDependencyCollector& DependencyCollector,
		FIntPoint RenderTargetSize,
		const FBox2D& Bounds,
		const TArray<FVoxelHeightExportChannelData>& Channels,
		const FVoxelWeakStackLayer& WeakLayer,
		int32 LOD,
		EPixelFormat Format,
		const TSharedPtr<TVoxelArray<uint8>>& OutValues);
};