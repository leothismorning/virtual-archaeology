// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Volume/VoxelVolumeSculptStamp.h"
#include "Sculpt/Volume/VoxelVolumeSculptData.h"
#include "Sculpt/Volume/VoxelVolumeSculptCache.h"
#include "Sculpt/Volume/VoxelVolumeSculptEditor.h"
#include "Sculpt/Volume/VoxelVolumeSculptInnerData.h"
#include "Sculpt/Volume/VoxelVolumeChunkTree.h"
#include "Sculpt/Volume/VoxelVolumeModifier.h"
#include "VoxelWorld.h"
#include "VoxelLayers.h"
#include "VoxelStackLayer.h"
#include "VoxelLayerStack.h"
#include "VoxelInvalidationCallstack.h"
#include "Surface/VoxelSurfaceTypeTable.h"

FVoxelVolumeSculptStamp::FVoxelVolumeSculptStamp()
{
	BlendMode = EVoxelVolumeBlendMode::Override;
}

FVoxelFuture FVoxelVolumeSculptStamp::ApplyModifier(const TSharedRef<const FVoxelVolumeModifier>& Modifier)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	const TSharedPtr<const FVoxelStampRuntime> Runtime = ResolveStampRuntime();
	if (!Runtime)
	{
		VOXEL_MESSAGE(Error, "Cannot sculpt: Stamp is not registered");
		return {};
	}

	UWorld* World = Runtime->GetWorld().Resolve();
	if (!ensureVoxelSlow(World))
	{
		VOXEL_MESSAGE(Error, "Cannot sculpt: World is null");
		return {};
	}

	if (!Layer)
	{
		VOXEL_MESSAGE(Error, "Cannot sculpt: Stamp has no layer assigned");
		return {};
	}

	TVoxelOptional<FVoxelStackLayer> StackLayer;
	if (StackOverride)
	{
		if (StackOverride->VolumeLayers.Contains(Layer))
		{
			StackLayer = FVoxelStackLayer(StackOverride, Layer);
		}

		if (!StackLayer)
		{
			VOXEL_MESSAGE(Error,
				"Cannot sculpt: Failed to find a matching layer in StackOverride.\n"
				"StackOverride: {0}\n"
				"StackOverride layers: {1}\n"
				"Stamp layer: {2}",
				StackOverride,
				StackOverride->VolumeLayers,
				Layer);

			return {};
		}
	}
	else
	{
		StackLayer = INLINE_LAMBDA -> TVoxelOptional<FVoxelStackLayer>
		{
			VOXEL_SCOPE_COUNTER("TActorRange<AVoxelWorld>");

			for (const AVoxelWorld* VoxelWorld : TActorRange<AVoxelWorld>(World))
			{
				UVoxelLayerStack* Stack = VoxelWorld->LayerStack;
				if (!Stack)
				{
					continue;
				}

				if (Stack->VolumeLayers.Contains(Layer))
				{
					return FVoxelStackLayer(Stack, Layer);
				}
			}

			return {};
		};

		if (!StackLayer)
		{
			TArray<const AVoxelWorld*> VoxelWorlds;
			for (const AVoxelWorld* VoxelWorld : TActorRange<AVoxelWorld>(World))
			{
				VoxelWorlds.Add(VoxelWorld);
			}

			VOXEL_MESSAGE(Error,
				"Cannot sculpt: Failed to find a Voxel World with a compatible stack.\n"
				"You need a Voxel World with a LayerStack containing the following layer in your scene: {0}\n"
				"If this sculpt stamp is not meant to be rendered with a Voxel World, manually set StackOverride\n"
				"Voxel Worlds checked: {0}",
				Layer,
				VoxelWorlds);

			return {};
		}
	}
	check(StackLayer);

	const FMatrix SculptToWorld = FScaleMatrix(Scale) * Runtime->GetLocalToWorld().ToMatrixWithScale();
	const FVoxelVolumeSculptDataId SculptDataId = GetData()->SculptDataId;

	if (!Cache ||
		Cache->SculptToWorld != SculptToWorld ||
		Cache->SculptDataId != SculptDataId)
	{
		Cache = MakeShared<FVoxelVolumeSculptCache>(SculptToWorld, SculptDataId);
	}

	check(Cache->SculptToWorld == SculptToWorld);
	check(Cache->SculptDataId == SculptDataId);

	const TSharedRef<FVoxelVolumeModifier> LocalModifier = Modifier->MakeSharedCopy();
	LocalModifier->Initialize_GameThread();

	const FVoxelInvalidationScope Scope(GetComponent());

	const TSharedRef<FVoxelVolumeSculptEditor> Editor = MakeShared<FVoxelVolumeSculptEditor>(
		bEnableDiffing && !bUseFastDistances,
		SculptToWorld,
		SculptDataId,
		BlendMode,
		FVoxelLayers::Get(World),
		FVoxelSurfaceTypeTable::Get(),
		StackLayer.GetValue(),
		Cache.ToSharedRef(),
		LocalModifier);

	return GetData()->AddTask(
		[Editor](FVoxelVolumeSculptInnerData& InnerData)
		{
			return Editor->DoWork(InnerData);
		},
		ResolveStampRuntime());
}

FVoxelFuture FVoxelVolumeSculptStamp::SetInnerData(const TSharedRef<const FVoxelVolumeSculptInnerData>& NewInnerData)
{
	bUseFastDistances = NewInnerData->bUseFastDistances;

	return GetData()->AddTask(
		[NewInnerData = NewInnerData](FVoxelVolumeSculptInnerData& InnerData)
		{
			InnerData.CopyFrom(*NewInnerData);
			return FVoxelBox::Infinite;
		},
		ResolveStampRuntime());
}

FVoxelFuture FVoxelVolumeSculptStamp::ClearSculptData()
{
	return GetData()->AddTask(
		[bUseFastDistances = bUseFastDistances](FVoxelVolumeSculptInnerData& InnerData)
		{
			InnerData.CopyFrom(FVoxelVolumeSculptInnerData(bUseFastDistances));
			return FVoxelBox::Infinite;
		},
		ResolveStampRuntime());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelVolumeSculptStamp::ClearCache()
{
	VOXEL_FUNCTION_COUNTER();

	Cache.Reset();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<FVoxelVolumeSculptData> FVoxelVolumeSculptStamp::GetData() const
{
	if (!PrivateData ||
		!ensure(PrivateData->bUseFastDistances == bUseFastDistances))
	{
		PrivateData = MakeShared<FVoxelVolumeSculptData>(bUseFastDistances);
	}
	return PrivateData.ToSharedRef();
}

void FVoxelVolumeSculptStamp::SetData(const TSharedRef<FVoxelVolumeSculptData>& NewData)
{
	PrivateData = NewData;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelVolumeSculptStamp::FixupProperties(const IVoxelStampComponentInterface* Interface)
{
	VOXEL_FUNCTION_COUNTER();

	Super::FixupProperties(Interface);

	if (PrivateData &&
		PrivateData->bUseFastDistances != bUseFastDistances)
	{
		PrivateData = MakeShared<FVoxelVolumeSculptData>(bUseFastDistances);
	}
}

void FVoxelVolumeSculptStamp::PostDuplicate()
{
	VOXEL_FUNCTION_COUNTER();

	PrivateData = MakeShared<FVoxelVolumeSculptData>(GetData()->GetInnerData());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void FVoxelVolumeSculptStamp::GetActorLabelInfo(FActorLabelInfo& Info) const
{
	Info.Identifier = "VolumeSculpt";
}

void FVoxelVolumeSculptStamp::GetPropertyInfo(FPropertyInfo& Info) const
{
	Info.bIsSmoothnessVisible = false;
	Info.bIsMetadataOverridesVisible = false;
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelVolumeSculptStampRuntime::Initialize(FVoxelDependencyCollector& DependencyCollector)
{
	const TSharedRef<const FVoxelVolumeSculptData> Data = Stamp.GetData();

	SculptDataId = Data->SculptDataId;
	Dependency = Data->Dependency;
	InnerData = Data->GetInnerData();

	if (Stamp.bUseFastDistances)
	{
		if (!InnerData->DistanceChunkTree_LQ->IsEmpty())
		{
			return true;
		}
	}
	else
	{
		if (!InnerData->DistanceChunkTree_HQ->IsEmpty())
		{
			return true;
		}
	}

	if (!InnerData->SurfaceTypeChunkTree->IsEmpty())
	{
		return true;
	}

	for (auto& It : InnerData->MetadataRefToChunkTree)
	{
		if (!It.Value->IsEmpty())
		{
			return true;
		}
	}

	return false;
}

FVoxelBox FVoxelVolumeSculptStampRuntime::GetLocalBounds() const
{
	FVoxelBox Bounds = FVoxelBox::InvertedInfinite;

	if (Stamp.bUseFastDistances)
	{
		if (!InnerData->DistanceChunkTree_LQ->IsEmpty())
		{
			Bounds += InnerData->DistanceChunkTree_LQ->GetBounds().Scale(Stamp.Scale);
		}
	}
	else
	{
		if (!InnerData->DistanceChunkTree_HQ->IsEmpty())
		{
			Bounds += InnerData->DistanceChunkTree_HQ->GetBounds().Scale(Stamp.Scale);
		}
	}

	if (!InnerData->SurfaceTypeChunkTree->IsEmpty())
	{
		Bounds += InnerData->SurfaceTypeChunkTree->GetBounds().Scale(Stamp.Scale);
	}

	for (const auto& It : InnerData->MetadataRefToChunkTree)
	{
		if (!It.Value->IsEmpty())
		{
			Bounds += It.Value->GetBounds().Scale(Stamp.Scale);
		}
	}

	return Bounds;
}

bool FVoxelVolumeSculptStampRuntime::ShouldFullyInvalidate(
	const FVoxelStampRuntime& PreviousRuntime,
	TVoxelArray<FVoxelBox>& OutLocalBoundsToInvalidate) const
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelVolumeSculptStampRuntime& TypedPreviousRuntime = CastStructChecked<FVoxelVolumeSculptStampRuntime>(PreviousRuntime);
	if (TypedPreviousRuntime.SculptDataId != SculptDataId)
	{
		return true;
	}

	// Invalidate any new bounds
	GetLocalBounds().Remove_Split(
		TypedPreviousRuntime.GetLocalBounds(),
		OutLocalBoundsToInvalidate);

	return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelVolumeSculptStampRuntime::Apply(
	const FVoxelVolumeBulkQuery& Query,
	const FVoxelVolumeTransform& StampToQuery) const
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelBox Bounds = StampToQuery.InverseTransform(Query.GetBounds()).Scale(1. / Stamp.Scale);
	Query.AddDependency(*Dependency, Bounds);

	if (AffectShape())
	{
		InnerData->ApplyShape(
			Query,
			StampToQuery,
			Stamp.Scale,
			Stamp.BlendMode,
			Stamp.bApplyOnVoid);
	}
}

void FVoxelVolumeSculptStampRuntime::Apply(
	const FVoxelVolumeSparseQuery& Query,
	const FVoxelVolumeTransform& StampToQuery) const
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelBox Bounds = StampToQuery.InverseTransform(Query.PositionBounds).Scale(1. / Stamp.Scale);
	Query.AddDependency(*Dependency, Bounds);

	if (AffectShape())
	{
		InnerData->ApplyShape(
			Query,
			StampToQuery,
			Stamp.Scale,
			Stamp.BlendMode,
			Stamp.bApplyOnVoid);
	}

	if (AffectSurfaceType() &&
		Query.bQuerySurfaceTypes)
	{
		InnerData->ApplySurfaceType(
			Query,
			StampToQuery,
			Stamp.Scale);
	}

	if (AffectMetadata())
	{
		for (const FVoxelMetadataRef& Metadata : Query.MetadatasToQuery)
		{
			InnerData->ApplyMetadata(
				Metadata,
				Query,
				StampToQuery,
				Stamp.Scale);
		}
	}
}