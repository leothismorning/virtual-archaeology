// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelDebugActor.h"
#include "VoxelQuery.h"
#include "VoxelLayers.h"
#include "Surface/VoxelSurfaceTypeTable.h"

#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

VOXEL_RUN_ON_STARTUP_GAME()
{
	Voxel::OnRefreshAll.AddLambda([]
	{
		ForEachObjectOfClass_Copy<AVoxelDebugActor>([&](AVoxelDebugActor& DebugActor)
		{
			DebugActor.Refresh();
		});
	});
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

AVoxelDebugActor::AVoxelDebugActor()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMesh(TEXT("/Engine/BasicShapes/Cube"));

	PrimaryActorTick.bCanEverTick = true;

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	StaticMeshComponent->SetStaticMesh(StaticMesh.Object);

	RootComponent = StaticMeshComponent;
}

void AVoxelDebugActor::Refresh()
{
	DependencyTracker.Reset();
}

void AVoxelDebugActor::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	SerializeVoxelVersion(Ar);
}

void AVoxelDebugActor::Tick(const float DeltaSeconds)
{
	VOXEL_FUNCTION_COUNTER();

	Super::Tick(DeltaSeconds);

	Settings.LOD = FMath::Clamp(Settings.LOD, 0, 25);
	Settings.Size = FMath::Clamp(Settings.Size, 8, 4096);
	Settings.Size = FVoxelUtilities::DivideCeil(Settings.Size, 2) * 2;

	if (!ParentMaterial)
	{
		ParentMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Voxel/Debug/VoxelDebugActorMaterial"));
		ensureVoxelSlow(ParentMaterial);
	}

	if (!LastFuture.IsComplete())
	{
		return;
	}

	if (GetActorTransform().Equals(LastTransform) &&
		Settings == LastSettings &&
		(DependencyTracker && !DependencyTracker->IsInvalidated()))
	{
		return;
	}

	LastTransform = GetActorTransform();
	LastSettings = Settings;

	const int32 Size = Settings.Size;
	const double VoxelSize = Settings.VoxelSize;
	const double Scale = VoxelSize * (Size << Settings.LOD);
	SetActorScale3D(FVector(Scale) / 100.f);
	SetActorRotation(FRotator::ZeroRotator);

	const FVoxelWeakStackLayer WeakLayer = FVoxelWeakStackLayer(Settings.Layer);
	const TSharedRef<FVoxelLayers> VoxelLayers = FVoxelLayers::Get(GetWorld());
	const TSharedRef<FVoxelSurfaceTypeTable> SurfaceTypeTable = FVoxelSurfaceTypeTable::Get();

	const TSharedRef<FVoxelDependencyCollector> DependencyCollector = MakeShared<FVoxelDependencyCollector>(STATIC_FNAME("AVoxelDebugActor"));

	const auto Sample = [&](const FIntVector& Offset, const FIntVector& DataSize)
	{
		return Voxel::AsyncTask([
			=,
			Settings = Settings,
			ActorLocation = GetActorLocation()]
		{
			const FVoxelQuery Query(
				Settings.LOD,
				*VoxelLayers,
				*SurfaceTypeTable,
				*DependencyCollector);

			return Query.SampleVolumeLayer(
				WeakLayer,
				ActorLocation + FVector(Size * Offset) / 2 * VoxelSize,
				DataSize,
				(1 << Settings.LOD) * VoxelSize);
		});
	};

	TVoxelArray<TVoxelFuture<FVoxelFloatBuffer>> Futures;
	Futures.Add(Sample(FIntVector(-1, -1, -1), FIntVector(1, Size, Size)));
	Futures.Add(Sample(FIntVector(+1, -1, -1), FIntVector(1, Size, Size)));
	Futures.Add(Sample(FIntVector(-1, -1, -1), FIntVector(Size, 1, Size)));
	Futures.Add(Sample(FIntVector(-1, +1, -1), FIntVector(Size, 1, Size)));
	Futures.Add(Sample(FIntVector(-1, -1, -1), FIntVector(Size, Size, 1)));
	Futures.Add(Sample(FIntVector(-1, -1, +1), FIntVector(Size, Size, 1)));

	ensure(LastFuture.IsComplete());
	LastFuture = FVoxelFuture(Futures).Then_AsyncThread([=, Settings = Settings, WeakThis = MakeWeakObjectPtr(this)]
	{
		VOXEL_FUNCTION_COUNTER();

		TVoxelStaticArray<TVoxelArray<FColor>, 6> AllColors;
		for (int32 Face = 0; Face < 6; Face++)
		{
			const FVoxelFloatBuffer& Distances = Futures[Face].GetValueChecked();
			ensure(Distances.Num() == Size * Size);

			TVoxelArray<FColor>& Colors = AllColors[Face];
			FVoxelUtilities::SetNumFast(Colors, Size * Size);

			for (int32 Index = 0; Index < Size * Size; Index++)
			{
				const float Distance = Distances[Index];

				if (FVoxelUtilities::IsNaN(Distance))
				{
					Colors[Index] = FColor::Purple;
					continue;
				}

				if (Settings.bGrayscale)
				{
					const float Color = Distance / VoxelSize * Settings.GrayscaleScale / 10000.f;
					Colors[Index] = FLinearColor(Color, Color, Color).ToFColor(true);
				}
				else
				{
					Colors[Index] = FVoxelUtilities::GetDistanceFieldColor(Distance / VoxelSize / Settings.ColorStep).ToFColor(true);
				}
			}
		}

		return Voxel::GameTask([=, AllColors = MoveTemp(AllColors)]
		{
			VOXEL_FUNCTION_COUNTER();

			AVoxelDebugActor* This = WeakThis.Get();
			if (!ensure(This))
			{
				return;
			}

			This->Textures.SetNum(6);

			for (int32 Face = 0; Face < 6; Face++)
			{
				TObjectPtr<UTexture2D>& Texture = This->Textures[Face];

				Texture = FVoxelTextureUtilities::CreateTexture2D(
					FName("VoxelDebugActor_" + FString::FromInt(Face)),
					Size,
					Size,
					true,
					TF_Bilinear,
					PF_B8G8R8A8,
					[&](TVoxelArrayView64<uint8> Data)
					{
						FVoxelUtilities::Memcpy(Data, MakeByteVoxelArrayView(AllColors[Face]));
					},
					Texture);
			}

			if (!This->Material)
			{
				This->Material = UMaterialInstanceDynamic::Create(This->ParentMaterial, nullptr);
				This->StaticMeshComponent->SetMaterial(0, This->Material);
			}

			This->Material->SetTextureParameterValue("TextureMinX", This->Textures[0]);
			This->Material->SetTextureParameterValue("TextureMaxX", This->Textures[1]);
			This->Material->SetTextureParameterValue("TextureMinY", This->Textures[2]);
			This->Material->SetTextureParameterValue("TextureMaxY", This->Textures[3]);
			This->Material->SetTextureParameterValue("TextureMinZ", This->Textures[4]);
			This->Material->SetTextureParameterValue("TextureMaxZ", This->Textures[5]);

			This->DependencyTracker = DependencyCollector->Finalize(nullptr, {});
		});
	});
}