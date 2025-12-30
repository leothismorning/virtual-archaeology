// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelActorFactories.h"
#include "VoxelWorld.h"
#include "VoxelSettings.h"
#include "VoxelStampActor.h"
#include "VoxelDebugActor.h"
#include "Shape/VoxelShape.h"
#include "VoxelStampComponent.h"
#include "Shape/VoxelShapeStamp.h"
#include "Heightmap/VoxelHeightmap.h"
#include "Heightmap/VoxelHeightmapStamp.h"
#include "Graphs/VoxelHeightGraph.h"
#include "Graphs/VoxelVolumeGraph.h"
#include "Graphs/VoxelHeightGraphStamp.h"
#include "Graphs/VoxelVolumeGraphStamp.h"
#include "Spline/VoxelHeightSplineGraph.h"
#include "Spline/VoxelHeightSplineStamp.h"
#include "Spline/VoxelVolumeSplineGraph.h"
#include "Spline/VoxelVolumeSplineStamp.h"
#include "StaticMesh/VoxelMeshStamp.h"
#include "StaticMesh/VoxelStaticMesh.h"
#include "Sculpt/Height/VoxelHeightSculptActor.h"
#include "Sculpt/Volume/VoxelVolumeSculptActor.h"

DEFINE_VOXEL_PLACEABLE_ITEM_FACTORY(UActorFactory_VoxelWorld);
DEFINE_VOXEL_PLACEABLE_ITEM_FACTORY(UActorFactory_VoxelStampActor);
DEFINE_VOXEL_PLACEABLE_ITEM_FACTORY(UActorFactory_VoxelDebugActor);
DEFINE_VOXEL_PLACEABLE_ITEM_FACTORY(UActorFactory_VoxelHeightSculptActor);
DEFINE_VOXEL_PLACEABLE_ITEM_FACTORY(UActorFactory_VoxelVolumeSculptActor);

UActorFactory_VoxelWorld::UActorFactory_VoxelWorld()
{
	DisplayName = INVTEXT("Voxel World");
	NewActorClass = AVoxelWorld::StaticClass();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UActorFactory_VoxelStampActor::UActorFactory_VoxelStampActor()
{
	DisplayName = INVTEXT("Voxel Stamp");
	NewActorClass = AVoxelStampActor::StaticClass();
}

bool UActorFactory_VoxelStampActor::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	// When dragging actor from spawn actor menu, the AssetData will be the actor class to spawn
	if (AssetData.GetAsset() == AVoxelStampActor::StaticClass())
	{
		return true;
	}

	const UClass* Class = AssetData.GetClass();
	if (!Class)
	{
		// Will make an empty stamp actor
		return true;
	}

	return
		Class->IsChildOf<UVoxelHeightmap>() ||
		Class->IsChildOf<UVoxelHeightGraph>() ||
		Class->IsChildOf<UVoxelVolumeGraph>() ||
		Class->IsChildOf<UVoxelHeightSplineGraph>() ||
		Class->IsChildOf<UVoxelVolumeSplineGraph>() ||
		Class->IsChildOf<UVoxelStaticMesh>();
}

void UActorFactory_VoxelStampActor::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);

	AVoxelStampActor& StampActor = *CastChecked<AVoxelStampActor>(NewActor);

	// Reset label set by FActorLabelUtilities::SetActorLabelUnique
	StampActor.LabelPrefix = {};

	if (UVoxelHeightmap* Heightmap = Cast<UVoxelHeightmap>(Asset))
	{
		FVoxelHeightmapStamp Stamp;
		Stamp.Heightmap = Heightmap;
		StampActor.SetStamp(Stamp);
	}

	if (UVoxelHeightGraph* Graph = Cast<UVoxelHeightGraph>(Asset))
	{
		FVoxelHeightGraphStamp Stamp;
		Stamp.Graph = Graph;
		StampActor.SetStamp(Stamp);
	}

	if (UVoxelVolumeGraph* Graph = Cast<UVoxelVolumeGraph>(Asset))
	{
		FVoxelVolumeGraphStamp Stamp;
		Stamp.Graph = Graph;
		StampActor.SetStamp(Stamp);
	}

	if (UVoxelHeightSplineGraph* Graph = Cast<UVoxelHeightSplineGraph>(Asset))
	{
		FVoxelHeightSplineStamp Stamp;
		Stamp.Graph = Graph;
		StampActor.SetStamp(Stamp);
	}

	if (UVoxelVolumeSplineGraph* Graph = Cast<UVoxelVolumeSplineGraph>(Asset))
	{
		FVoxelVolumeSplineStamp Stamp;
		Stamp.Graph = Graph;
		StampActor.SetStamp(Stamp);
	}

	if (UVoxelStaticMesh* Mesh = Cast<UVoxelStaticMesh>(Asset))
	{
		FVoxelMeshStamp Stamp;
		Stamp.NewMesh = Mesh;
		StampActor.SetStamp(Stamp);
	}

	if (!StampActor.GetStamp())
	{
		FVoxelMeshStamp Stamp;
		StampActor.SetStamp(Stamp);
	}

	if (const FVoxelStampRef Stamp = StampActor.GetStamp())
	{
		int32 NewPriority = 0;
		ForEachObjectOfClass<UVoxelStampComponent>([&](const UVoxelStampComponent& Component)
		{
			if (NewActor->GetWorld() != Component.GetWorld())
			{
				return;
			}

			const FVoxelStampRef& OtherStamp = Component.GetStamp();
			if (!OtherStamp)
			{
				return;
			}

			NewPriority = FMath::Max(NewPriority, OtherStamp->Priority + 1);
		});

		Stamp->Priority = NewPriority;
	}

	if (FVoxelHeightStamp* Stamp = StampActor.GetStamp().As<FVoxelHeightStamp>())
	{
		Stamp->Layer = GetDefault<UVoxelSettings>()->DefaultHeightLayer.LoadSynchronous();
	}
	if (FVoxelVolumeStamp* Stamp = StampActor.GetStamp().As<FVoxelVolumeStamp>())
	{
		Stamp->Layer = GetDefault<UVoxelSettings>()->DefaultVolumeLayer.LoadSynchronous();
	}

	// Ensure preview & label is up to date
	StampActor.GetStampComponent().PostEditChange();
	AVoxelWorld::CreateNewIfNeeded_EditorOnly(NewActor);
}

UObject* UActorFactory_VoxelStampActor::GetAssetFromActorInstance(AActor* ActorInstance)
{
	const AVoxelStampActor* TypedActor = Cast<AVoxelStampActor>(ActorInstance);
	if (!ensure(TypedActor))
	{
		return nullptr;
	}

	if (const TSharedPtr<const FVoxelHeightmapStamp> Stamp = TypedActor->GetStamp().ToSharedPtr<FVoxelHeightmapStamp>())
	{
		return Stamp->Heightmap;
	}

	if (const TSharedPtr<const FVoxelMeshStamp> Stamp = TypedActor->GetStamp().ToSharedPtr<FVoxelMeshStamp>())
	{
		return Stamp->NewMesh;
	}

	if (const TSharedPtr<const FVoxelHeightGraphStamp> Stamp = TypedActor->GetStamp().ToSharedPtr<FVoxelHeightGraphStamp>())
	{
		return Stamp->Graph;
	}

	if (const TSharedPtr<const FVoxelVolumeGraphStamp> Stamp = TypedActor->GetStamp().ToSharedPtr<FVoxelVolumeGraphStamp>())
	{
		return Stamp->Graph;
	}

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UActorFactory_VoxelDebugActor::UActorFactory_VoxelDebugActor()
{
	DisplayName = INVTEXT("Voxel Debug Actor");
	NewActorClass = AVoxelDebugActor::StaticClass();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UActorFactory_VoxelHeightSculptActor::UActorFactory_VoxelHeightSculptActor()
{
	DisplayName = INVTEXT("Voxel Height Sculpt Actor");
	NewActorClass = AVoxelHeightSculptActor::StaticClass();
}

UActorFactory_VoxelVolumeSculptActor::UActorFactory_VoxelVolumeSculptActor()
{
	DisplayName = INVTEXT("Voxel Volume Sculpt Actor");
	NewActorClass = AVoxelVolumeSculptActor::StaticClass();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UActorFactory_VoxelPlaceStampActor::UActorFactory_VoxelPlaceStampActor()
{
	DisplayName = INVTEXT("Voxel Stamp Actor");
	NewActorClass = AVoxelStampActor::StaticClass();
}

bool UActorFactory_VoxelPlaceStampActor::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	if (!AssetData.IsValid())
	{
		return true;
	}

	if (!AssetData.GetClass()->IsChildOf<UScriptStruct>())
	{
		return false;
	}

	const UScriptStruct* Struct = Cast<UScriptStruct>(AssetData.GetAsset());
	return
		Struct->IsChildOf<FVoxelStamp>() ||
		Struct->IsChildOf<FVoxelShape>();
}

void UActorFactory_VoxelPlaceStampActor::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);

	int32 NewPriority = 0;
	ForEachObjectOfClass<UVoxelStampComponent>([&](const UVoxelStampComponent& Component)
	{
		if (NewActor->GetWorld() != Component.GetWorld())
		{
			return;
		}

		const FVoxelStampRef& Stamp = Component.GetStamp();
		if (!Stamp)
		{
			return;
		}

		NewPriority = FMath::Max(NewPriority, Stamp->Priority + 1);
	});

	AVoxelStampActor& StampActor = *CastChecked<AVoxelStampActor>(NewActor);

	if (UScriptStruct* Struct = Cast<UScriptStruct>(Asset))
	{
		FVoxelStampRef StampRef;
		if (Struct->IsChildOf<FVoxelStamp>())
		{
			StampRef.SetStruct_Editor(Struct);
		}
		else if (Struct->IsChildOf<FVoxelShape>())
		{
			FVoxelShapeStamp Stamp;
			Stamp.Shape = TVoxelInstancedStruct<FVoxelShape>(Struct);

			StampRef = FVoxelStampRef::New(Stamp);
		}

		if (StampRef)
		{
			StampRef->Priority = NewPriority;
		}

		StampActor.SetStamp(StampRef);
	}

	// Ensure preview & label is up to date
	StampActor.GetStampComponent().PostEditChange();
	AVoxelWorld::CreateNewIfNeeded_EditorOnly(NewActor);
}