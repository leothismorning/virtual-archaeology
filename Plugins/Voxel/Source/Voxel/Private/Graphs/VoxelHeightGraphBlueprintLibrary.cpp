// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Graphs/VoxelHeightGraphBlueprintLibrary.h"
#include "VoxelQuery.h"
#include "VoxelLayers.h"
#include "Buffer/VoxelBaseBuffers.h"
#include "Surface/VoxelSurfaceTypeTable.h"

#include "TextureResource.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/TextureRenderTarget2D.h"

bool UVoxelHeightGraphBlueprintLibrary::SyncExportDataToRenderTarget(
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
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	TArray<FVoxelHeightExportChannelData> Channels =
	{
		R,
		G,
		B,
		A
	};

	for (FVoxelHeightExportChannelData& Channel : Channels)
	{
		Channel.MetadataRef = FVoxelFloatMetadataRef(Channel.Metadata);
	}

	if (!IsValidDataForExport(World, RenderTarget, Channels))
	{
		return false;
	}

	const FIntPoint RenderTargetSize = FIntPoint(RenderTarget->SizeX, RenderTarget->SizeY);

	const TSharedRef<TVoxelArray<uint8>> Values = MakeShared<TVoxelArray<uint8>>();
	FVoxelUtilities::SetNumZeroed(*Values, RenderTarget->SizeX * RenderTarget->SizeY * GPixelFormats[RenderTarget->GetFormat()].BlockBytes);

	const TSharedRef<FVoxelLayers> Layers = FVoxelLayers::Get(World);
	const TSharedRef<FVoxelSurfaceTypeTable> SurfaceTypeTable = FVoxelSurfaceTypeTable::Get();

	ReadValues(
		*Layers,
		*SurfaceTypeTable,
		FVoxelDependencyCollector::Null,
		RenderTargetSize,
		Bounds,
		Channels,
		FVoxelWeakStackLayer(Layer),
		LOD,
		RenderTarget->GetFormat(),
		Values);

	if (!ensure(Values->Num() == RenderTarget->SizeX * RenderTarget->SizeY * GPixelFormats[RenderTarget->GetFormat()].BlockBytes))
	{
		return false;
	}

	FTextureResource* Resource = RenderTarget->GetResource();
	if (!Resource)
	{
		return false;
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

	return true;
}

bool UVoxelHeightGraphBlueprintLibrary::IsValidDataForExport(
	const UWorld* World,
	const UTextureRenderTarget2D* RenderTarget,
	const TArray<FVoxelHeightExportChannelData>& Channels)
{
	if (!RenderTarget)
	{
		VOXEL_MESSAGE(Error, "RenderTarget is null");
		return false;
	}

	if (!World)
	{
		VOXEL_MESSAGE(Error, "World is null");
		return false;
	}

	if (!ensure(Channels.Num() == 4))
	{
		VOXEL_MESSAGE(Error, "Invalid channels");
		return false;
	}

	TSet<ETextureRenderTargetFormat> ValidFormats;
	if (Channels[3].ChannelType != EVoxelHeightExportChannelType::None ||
		Channels[2].ChannelType != EVoxelHeightExportChannelType::None)
	{
		ValidFormats = { RTF_RGBA16f, RTF_RGBA32f };
	}
	else if (Channels[1].ChannelType != EVoxelHeightExportChannelType::None)
	{
		ValidFormats = { RTF_RGBA16f, RTF_RGBA32f, RTF_RG16f, RTF_RG32f };
	}
	else if (Channels[0].ChannelType != EVoxelHeightExportChannelType::None)
	{
		ValidFormats = { RTF_RGBA16f, RTF_RGBA32f, RTF_RG16f, RTF_RG32f, RTF_R16f, RTF_R32f };
	}
	else
	{
		return false;
	}

	if (!ValidFormats.Contains(RenderTarget->RenderTargetFormat))
	{
		const UEnum* Enum = StaticEnumFast<ETextureRenderTargetFormat>();
		TArray<FText> ValidFormatNames;
		for (const ETextureRenderTargetFormat& Format : ValidFormats)
		{
			ValidFormatNames.Add(Enum->GetDisplayNameTextByValue(Format));
		}

		VOXEL_MESSAGE(Error, "Invalid render target format. Valid formats: {0}.", ValidFormatNames);
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelHeightGraphBlueprintLibrary::ReadValues(
	const FVoxelLayers& Layers,
	const FVoxelSurfaceTypeTable& SurfaceTypeTable,
	FVoxelDependencyCollector& DependencyCollector,
	const FIntPoint RenderTargetSize,
	const FBox2D& Bounds,
	const TArray<FVoxelHeightExportChannelData>& Channels,
	const FVoxelWeakStackLayer& WeakLayer,
	const int32 LOD,
	const EPixelFormat Format,
	const TSharedPtr<TVoxelArray<uint8>>& OutValues)
{
	VOXEL_FUNCTION_COUNTER();

	const FVector2D BoundsSize = FVector2D(Bounds.GetSize());
	const FVector2D Step = BoundsSize / RenderTargetSize;

	FVoxelDoubleVector2DBuffer Positions;
	Positions.Allocate(RenderTargetSize.X * RenderTargetSize.Y);

	for (int32 X = 0; X < RenderTargetSize.X; X++)
	{
		for (int32 Y = 0; Y < RenderTargetSize.Y; Y++)
		{
			const int32 Index = FVoxelUtilities::Get2DIndex<int32>(RenderTargetSize, X, Y);

			Positions.X.Set(Index, Bounds.Min.X + Step.X * X);
			Positions.Y.Set(Index, Bounds.Min.Y + Step.Y * Y);
		}
	}

	TVoxelMap<FVoxelMetadataRef, TSharedRef<FVoxelFloatBuffer>> MetadataToBuffer;
	for (const FVoxelHeightExportChannelData& Channel : Channels)
	{
		if (Channel.ChannelType != EVoxelHeightExportChannelType::Metadata ||
			!Channel.MetadataRef.IsValid())
		{
			continue;
		}
		checkVoxelSlow(Channel.MetadataRef.GetMetadata().Resolve() == Channel.Metadata);

		const TSharedRef<FVoxelFloatBuffer> Buffer = MakeShared<FVoxelFloatBuffer>();
		Buffer->AllocateZeroed(Positions.Num());
		MetadataToBuffer.Add_EnsureNew(Channel.MetadataRef, Buffer);
	}

	const FVoxelQuery Query(
		LOD,
		Layers,
		SurfaceTypeTable,
		DependencyCollector);

	const FVoxelFloatBuffer ReadHeights = Query.SampleHeightLayer(
		WeakLayer,
		Positions,
		{},
		MetadataToBuffer);

	const int32 NumComponents = GPixelFormats[Format].NumComponents;
	const bool bHalfFloat = Format == PF_R16F || Format == PF_G16R16F || Format == PF_FloatRGBA;
	ensure(bHalfFloat || Format == PF_R32_FLOAT || Format == PF_G32R32F || Format == PF_A32B32G32R32F);

	const auto WriteValues = [&](const FVoxelFloatBuffer& Values, const int32 Offset)
	{
		if (bHalfFloat)
		{
			const TVoxelArrayView<FFloat16> Data = OutValues->View<FFloat16>();
			for (int32 Index = 0; Index < RenderTargetSize.X * RenderTargetSize.Y; Index++)
			{
				Data[Index * NumComponents + Offset] = Values[Index];
			}
		}
		else
		{
			const TVoxelArrayView<float> Data = OutValues->View<float>();
			for (int32 Index = 0; Index < RenderTargetSize.X * RenderTargetSize.Y; Index++)
			{
				Data[Index * NumComponents + Offset] = Values[Index];
			}
		}
	};

	for (int32 Index = 0; Index < NumComponents; Index++)
	{
		const FVoxelHeightExportChannelData& Channel = Channels[Index];
		if (Channel.ChannelType == EVoxelHeightExportChannelType::Metadata)
		{
			if (!Channel.MetadataRef.IsValid())
			{
				continue;
			}

			WriteValues(*MetadataToBuffer[Channel.MetadataRef], Index);
		}
		else if (Channel.ChannelType == EVoxelHeightExportChannelType::Height)
		{
			WriteValues(ReadHeights, Index);
		}
	}
}