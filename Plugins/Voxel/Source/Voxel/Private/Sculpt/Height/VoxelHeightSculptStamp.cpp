// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Height/VoxelHeightSculptStamp.h"
#include "Sculpt/Height/VoxelHeightSculptData.h"
#include "Sculpt/Height/VoxelHeightSculptCache.h"
#include "Sculpt/Height/VoxelHeightSculptEditor.h"
#include "Sculpt/Height/VoxelHeightSculptInnerData.h"
#include "Sculpt/Height/VoxelHeightChunkTree.h"
#include "Sculpt/Height/VoxelHeightModifier.h"
#include "VoxelWorld.h"
#include "VoxelLayers.h"
#include "VoxelStackLayer.h"
#include "VoxelLayerStack.h"
#include "VoxelInvalidationCallstack.h"
#include "Surface/VoxelSurfaceTypeTable.h"

FVoxelHeightSculptStamp::FVoxelHeightSculptStamp()
	: Data(MakeShared<FVoxelHeightSculptData>())
{
	BlendMode = EVoxelHeightBlendMode::Override;
}

FVoxelFuture FVoxelHeightSculptStamp::ApplyModifier(const TSharedRef<const FVoxelHeightModifier>& Modifier)
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
		if (StackOverride->HeightLayers.Contains(Layer))
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
				StackOverride->HeightLayers,
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

				if (Stack->HeightLayers.Contains(Layer))
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

	const FTransform2d SculptToWorld = FTransform2d(FScale2d(ScaleXY)) * FVoxelUtilities::MakeTransform2(Runtime->GetLocalToWorld());
	const FVoxelHeightSculptDataId SculptDataId = GetData()->SculptDataId;

	if (!Cache ||
		Cache->SculptToWorld != SculptToWorld ||
		Cache->SculptDataId != SculptDataId)
	{
		Cache = MakeShared<FVoxelHeightSculptCache>(SculptToWorld, SculptDataId);
	}

	check(Cache->SculptToWorld == SculptToWorld);
	check(Cache->SculptDataId == SculptDataId);

	const TSharedRef<FVoxelHeightModifier> LocalModifier = Modifier->MakeSharedCopy();
	LocalModifier->Initialize_GameThread();

	const FVoxelInvalidationScope Scope(GetComponent());

	const TSharedRef<FVoxelHeightSculptEditor> Editor = MakeShared<FVoxelHeightSculptEditor>(
		bRelativeHeight,
		SculptToWorld,
		Runtime->GetLocalToWorld().GetScale3D().Z,
		Runtime->GetLocalToWorld().GetLocation().Z,
		SculptDataId,
		BlendMode,
		FVoxelLayers::Get(World),
		FVoxelSurfaceTypeTable::Get(),
		StackLayer.GetValue(),
		Cache.ToSharedRef(),
		LocalModifier);

	return Data->AddTask(
		[Editor](FVoxelHeightSculptInnerData& InnerData)
		{
			return Editor->DoWork(InnerData);
		},
		ResolveStampRuntime());
}

FVoxelFuture FVoxelHeightSculptStamp::SetInnerData(const TSharedRef<const FVoxelHeightSculptInnerData>& NewInnerData)
{
	return GetData()->AddTask(
		[NewInnerData = NewInnerData](FVoxelHeightSculptInnerData& InnerData)
		{
			InnerData.CopyFrom(*NewInnerData);
			return FVoxelBox2D::Infinite;
		},
		ResolveStampRuntime());
}

FVoxelFuture FVoxelHeightSculptStamp::ClearSculptData()
{
	return GetData()->AddTask(
		[](FVoxelHeightSculptInnerData& InnerData)
		{
			InnerData.CopyFrom(FVoxelHeightSculptInnerData());
			return FVoxelBox2D::Infinite;
		},
		ResolveStampRuntime());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelHeightSculptStamp::ClearCache()
{
	VOXEL_FUNCTION_COUNTER();

	Cache.Reset();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<FVoxelHeightSculptData> FVoxelHeightSculptStamp::GetData() const
{
	return Data;
}

void FVoxelHeightSculptStamp::SetData(const TSharedRef<FVoxelHeightSculptData>& NewData)
{
	Data = NewData;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelHeightSculptStamp::PostDuplicate()
{
	VOXEL_FUNCTION_COUNTER();

	Data = MakeShared<FVoxelHeightSculptData>(Data->GetInnerData());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void FVoxelHeightSculptStamp::GetActorLabelInfo(FActorLabelInfo& Info) const
{
	Info.Identifier = "HeightSculpt";
}

void FVoxelHeightSculptStamp::GetPropertyInfo(FPropertyInfo& Info) const
{
	Info.bIsSmoothnessVisible = false;
	Info.bIsMetadataOverridesVisible = false;
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelHeightSculptStampRuntime::Initialize(FVoxelDependencyCollector& DependencyCollector)
{
	const TSharedRef<const FVoxelHeightSculptData> Data = Stamp.GetData();

	SculptDataId = Data->SculptDataId;
	Dependency = Data->Dependency;
	InnerData = Data->GetInnerData();

	if (!InnerData->HeightChunkTree->IsEmpty() ||
		!InnerData->SurfaceTypeChunkTree->IsEmpty())
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

FVoxelBox FVoxelHeightSculptStampRuntime::GetLocalBounds() const
{
	FVoxelBox2D Bounds = FVoxelBox2D::InvertedInfinite;

	if (!InnerData->HeightChunkTree->IsEmpty())
	{
		Bounds += InnerData->HeightChunkTree->GetBounds().Scale(Stamp.ScaleXY);
	}

	if (!InnerData->SurfaceTypeChunkTree->IsEmpty())
	{
		Bounds += InnerData->SurfaceTypeChunkTree->GetBounds().Scale(Stamp.ScaleXY);
	}

	for (const auto& It : InnerData->MetadataRefToChunkTree)
	{
		if (!It.Value->IsEmpty())
		{
			Bounds += It.Value->GetBounds().Scale(Stamp.ScaleXY);
		}
	}

	if (InnerData->MinHeight < InnerData->MaxHeight)
	{
		return Bounds.ToBox3D(InnerData->MinHeight, InnerData->MaxHeight);
	}

	return Bounds.ToBox3D(-1.f, 1.f);
}

bool FVoxelHeightSculptStampRuntime::HasRelativeHeightRange() const
{
	return Stamp.bRelativeHeight;
}

bool FVoxelHeightSculptStampRuntime::ShouldFullyInvalidate(
	const FVoxelStampRuntime& PreviousRuntime,
	TVoxelArray<FVoxelBox>& OutLocalBoundsToInvalidate) const
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelHeightSculptStampRuntime& TypedPreviousRuntime = CastStructChecked<FVoxelHeightSculptStampRuntime>(PreviousRuntime);
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

void FVoxelHeightSculptStampRuntime::Apply(
	const FVoxelHeightBulkQuery& Query,
	const FVoxelHeightTransform& StampToQuery) const
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelBox2D Bounds = StampToQuery.InverseTransform(Query.GetBounds()).Scale(1. / Stamp.ScaleXY);
	Query.AddDependency(*Dependency, Bounds);

	if (AffectShape())
	{
		InnerData->ApplyShape(
			Query,
			StampToQuery,
			Stamp.ScaleXY,
			Stamp.BlendMode,
			Stamp.bApplyOnVoid,
			Stamp.bRelativeHeight);
	}
}

void FVoxelHeightSculptStampRuntime::Apply(
	const FVoxelHeightSparseQuery& Query,
	const FVoxelHeightTransform& StampToQuery) const
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelBox2D Bounds = StampToQuery.InverseTransform(Query.PositionBounds).Scale(1. / Stamp.ScaleXY);
	Query.AddDependency(*Dependency, Bounds);

	if (AffectShape())
	{
		InnerData->ApplyShape(
			Query,
			StampToQuery,
			Stamp.ScaleXY,
			Stamp.BlendMode,
			Stamp.bApplyOnVoid,
			Stamp.bRelativeHeight);
	}

	if (AffectSurfaceType() &&
		Query.bQuerySurfaceTypes)
	{
		InnerData->ApplySurfaceType(
			Query,
			StampToQuery,
			Stamp.ScaleXY);
	}

	if (AffectMetadata())
	{
		for (const FVoxelMetadataRef& Metadata : Query.MetadatasToQuery)
		{
			InnerData->ApplyMetadata(
				Metadata,
				Query,
				StampToQuery,
				Stamp.ScaleXY);
		}
	}
}