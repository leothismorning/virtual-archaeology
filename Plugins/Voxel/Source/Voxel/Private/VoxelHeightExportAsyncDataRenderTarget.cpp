// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelHeightExportAsyncDataRenderTarget.h"
#include "Graphs/VoxelHeightGraphBlueprintLibrary.h"
#include "VoxelLayers.h"
#include "Surface/VoxelSurfaceTypeTable.h"

#include "TextureResource.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/TextureRenderTarget2D.h"

UVoxelHeightExportAsyncDataRenderTarget* UVoxelHeightExportAsyncDataRenderTarget::AsyncExportDataToRenderTarget(
	UObject* WorldContextObject,
	const FBox2D& Bounds,
	UTextureRenderTarget2D* RenderTarget,
	const FVoxelStackHeightLayer Layer,
	const FVoxelHeightExportChannelData R,
	const FVoxelHeightExportChannelData G,
	const FVoxelHeightExportChannelData B,
	const FVoxelHeightExportChannelData A,
	const int32 LOD)
{
	UVoxelHeightExportAsyncDataRenderTarget* Action = NewObject<UVoxelHeightExportAsyncDataRenderTarget>();
	Action->Bounds = Bounds;
	Action->Channels = { R, G, B, A };
	Action->WeakLayer = FVoxelWeakStackLayer(Layer);
	Action->LOD = LOD;
	Action->WeakWorld = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	Action->RenderTarget = RenderTarget;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelHeightExportAsyncDataRenderTarget::Activate()
{
	Super::Activate();

	UWorld* World = WeakWorld.Resolve();
	if (!ensure(World))
	{
		return;
	}

	if (!UVoxelHeightGraphBlueprintLibrary::IsValidDataForExport(World, RenderTarget, Channels))
	{
		ExecuteCompleted(false);
		return;
	}

	ReadData(
		FVoxelLayers::Get(World),
		FVoxelSurfaceTypeTable::Get());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelHeightExportAsyncDataRenderTarget::ReadData(
	const TSharedRef<FVoxelLayers>& Layers,
	const TSharedRef<FVoxelSurfaceTypeTable>& SurfaceTypeTable)
{
	if (ActiveFuture.IsSet())
	{
		Voxel::FlushGameTasks();
		return;
	}

	const FIntPoint RenderTargetSize = FIntPoint(RenderTarget->SizeX, RenderTarget->SizeY);

	Values = MakeShared<TVoxelArray<uint8>>();
	FVoxelUtilities::SetNumZeroed(*Values, RenderTarget->SizeX * RenderTarget->SizeY * GPixelFormats[RenderTarget->GetFormat()].BlockBytes);

	OnFinishedReading.AddLambda(MakeWeakObjectPtrLambda(this, [this](const bool bSuccess)
	{
		FinishedReading(bSuccess);
	}));

	for (FVoxelHeightExportChannelData& Channel : Channels)
	{
		Channel.MetadataRef = FVoxelFloatMetadataRef(Channel.Metadata);
	}

	ActiveFuture = Voxel::AsyncTask([
		Layers,
		SurfaceTypeTable,
		RenderTargetSize,
		Bounds = Bounds,
		Channels = Channels,
		WeakLayer = WeakLayer,
		LOD = LOD,
		Format = RenderTarget->GetFormat(),
		Delegate = OnFinishedReading,
		WeakValues = MakeWeakPtr(Values)]
	{
		const TSharedPtr<TVoxelArray<uint8>> OutValues = WeakValues.Pin();
		if (!ensure(OutValues))
		{
			return Voxel::GameTask([Delegate]
			{
				Delegate.Broadcast(false);
			});
		}

		UVoxelHeightGraphBlueprintLibrary::ReadValues(
			*Layers,
			*SurfaceTypeTable,
			FVoxelDependencyCollector::Null,
			RenderTargetSize,
			Bounds,
			Channels,
			WeakLayer,
			LOD,
			Format,
			OutValues);

		return Voxel::GameTask([=]
		{
			VOXEL_FUNCTION_COUNTER();
			check(IsInGameThread());

			Delegate.Broadcast(true);
		});
	});
}

void UVoxelHeightExportAsyncDataRenderTarget::FinishedReading(const bool bSuccess)
{
	if (!bSuccess)
	{
		VOXEL_MESSAGE(Error, "Failed to export data");
		ExecuteCompleted(false);
		return;
	}

	if (!ensure(Values->Num() == RenderTarget->SizeX * RenderTarget->SizeY * GPixelFormats[RenderTarget->GetFormat()].BlockBytes))
	{
		VOXEL_MESSAGE(Error, "Invalid RenderTarget size");
		ExecuteCompleted(false);
		return;
	}

	if (!ensure(IsInGameThread()))
	{
		return;
	}

	FTextureResource* Resource = RenderTarget->GetResource();
	if (!Resource)
	{
		ExecuteCompleted(false);
		return;
	}

	Voxel::RenderTask(
	[
		Resource,
		Format = RenderTarget->GetFormat(),
		NewValues = Values,
		SizeX = RenderTarget->SizeX,
		SizeY = RenderTarget->SizeY](FRHICommandListImmediate& RHICmdList)
	{
		const TVoxelArrayView<uint8> Array = MakeVoxelArrayView(*NewValues).ReinterpretAs<uint8>();

		RHICmdList.UpdateTexture2D(
			Resource->GetTexture2DRHI(),
			0,
			FUpdateTextureRegion2D(0, 0, 0, 0, SizeX, SizeY),
			SizeX * GPixelFormats[Format].BlockBytes,
			Array.GetData());
	});

	FlushRenderingCommands();
	ExecuteCompleted(true);
}

void UVoxelHeightExportAsyncDataRenderTarget::ExecuteCompleted(const bool bSuccess)
{
	if (!ensure(IsInGameThread()))
	{
		return;
	}

	FEditorScriptExecutionGuard AllowScripts;
	Finish.Broadcast(bSuccess);
	SetReadyToDestroy();
}