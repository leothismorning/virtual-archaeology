// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Graphs/VoxelSampleStampNodes.h"
#include "Graphs/VoxelStampGraphParameters.h"
#include "VoxelQuery.h"
#include "VoxelLayers.h"
#include "VoxelMetadata.h"
#include "VoxelGraphMigration.h"

VOXEL_RUN_ON_STARTUP_GAME()
{
	REGISTER_VOXEL_NODE_MIGRATION("SampleHeightStamp", FVoxelNode_SampleHeightStamp);
	REGISTER_VOXEL_NODE_MIGRATION("SampleVolumeStamp", FVoxelNode_SampleVolumeStamp);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelNode_SampleStamp::FVoxelNode_SampleStamp()
{
	FixupMetadataPins();
}

void FVoxelNode_SampleStamp::Initialize(FInitializer& Initializer)
{
	VOXEL_FUNCTION_COUNTER();

	for (FMetadataPin& MetadataPin : MetadataPins)
	{
		Initializer.InitializePinRef(MetadataPin.PinRef);
	}
}

void FVoxelNode_SampleStamp::PostSerialize()
{
	Super::PostSerialize();

	FixupMetadataPins();
}

#if WITH_EDITOR
FVoxelNode::EPostEditChange FVoxelNode_SampleStamp::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.GetPropertyName() == GET_OWN_MEMBER_NAME(MetadatasToQuery))
	{
		FixupMetadataPins();
		return EPostEditChange::Reconstruct;
	}

	return EPostEditChange::None;
}
#endif

void FVoxelNode_SampleStamp::FixupMetadataPins()
{
	VOXEL_FUNCTION_COUNTER();

	for (const FMetadataPin& MetadataPin : MetadataPins)
	{
		RemovePin(MetadataPin.PinRef.GetName());
	}
	MetadataPins.Reset();

	MetadatasToQuery = TVoxelSet<TObjectPtr<UVoxelMetadata>>(MetadatasToQuery).Array();

	for (UVoxelMetadata* Metadata : MetadatasToQuery)
	{
		if (!Metadata)
		{
			continue;
		}

		MetadataPins.Add(FMetadataPin
		{
			FVoxelMetadataRef(Metadata),
			CreateOutputPin(
				Metadata->GetInnerType().GetBufferType(),
				Metadata->GetFName(),
				VOXEL_PIN_METADATA(
					void,
					nullptr,
					DisplayName(Metadata->GetName()),
					Category("Metadata"))),
		});
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNode_SampleHeightStampBase::ComputeImpl(
	const FVoxelGraphQuery& Query,
	const FVoxelDoubleVector2DBuffer& Position,
	const FVoxelFloatBuffer& PreviousHeight,
	const TSharedPtr<const FVoxelHeightStampRuntime>& StampRuntime) const
{
	const int32 Num = ComputeVoxelBuffersNum(Position, PreviousHeight);

	VOXEL_FUNCTION_COUNTER_NUM(Num);
	FVoxelNodeStatScope StatScope(*this, Num);

	FVoxelDoubleVector2DBuffer LocalPosition = Position;
	LocalPosition.ExpandConstants(Num);

	if (!StampRuntime)
	{
		VOXEL_MESSAGE(Error, "{0}: Invalid stamp", this);
		return;
	}

	const FVoxelGraphParameters::FQuery* QueryParameter = Query->FindParameter<FVoxelGraphParameters::FQuery>();
	if (!QueryParameter)
	{
		VOXEL_MESSAGE(Error, "{0}: Cannot sample stamp here, no query", this);
		return;
	}

	FVoxelSurfaceTypeBlendBuffer SurfaceTypes;
	if (SurfaceTypePin.ShouldCompute())
	{
		SurfaceTypes.AllocateZeroed(Num);
	}

	TVoxelMap<FVoxelMetadataRef, TSharedRef<FVoxelBuffer>> MetadataToBuffer;
	MetadataToBuffer.Reserve(MetadataPins.Num());

	for (const FMetadataPin& MetadataPin : MetadataPins)
	{
		if (MetadataPin.PinRef.ShouldCompute())
		{
			MetadataToBuffer.Add_EnsureNew(
				MetadataPin.MetadataRef,
				MetadataPin.MetadataRef.MakeDefaultBuffer(Num));
		}
	}

	FVoxelFloatBuffer Height;
	Height.Allocate(Num);
	Height.CopyFrom(PreviousHeight, 0, 0, Num);

	const FVoxelQuery VoxelQuery(
		0,
		FVoxelLayers::Empty(),
		QueryParameter->Query.SurfaceTypeTable,
		Query->Context.DependencyCollector);

	const FVoxelHeightSparseQuery SparseQuery = FVoxelHeightSparseQuery::Create(
		VoxelQuery,
		Height.View(),
		SurfaceTypes.View(),
		MetadataToBuffer,
		Position,
		SurfaceTypes.Num() > 0,
		MetadataToBuffer.KeyArray());

	StampRuntime->Apply(
		SparseQuery,
		{});

	HeightPin.Set(Query, Height);

	if (SurfaceTypePin.ShouldCompute())
	{
		SurfaceTypePin.Set(Query, SurfaceTypes);
	}

	for (const FMetadataPin& MetadataPin : MetadataPins)
	{
		if (MetadataPin.PinRef.ShouldCompute())
		{
			MetadataPin.PinRef.Set(Query, FVoxelRuntimePinValue::Make(MetadataToBuffer[MetadataPin.MetadataRef]));
		}
	}
}

void FVoxelNode_SampleVolumeStampBase::ComputeImpl(
	const FVoxelGraphQuery& Query,
	const FVoxelDoubleVectorBuffer& Position,
	const FVoxelFloatBuffer& PreviousDistance,
	const TSharedPtr<const FVoxelVolumeStampRuntime>& StampRuntime) const
{
	if (!StampRuntime)
	{
		VOXEL_MESSAGE(Error, "{0}: Invalid stamp", this);
		return;
	}

	const FVoxelGraphParameters::FQuery* QueryParameter = Query->FindParameter<FVoxelGraphParameters::FQuery>();
	if (!QueryParameter)
	{
		VOXEL_MESSAGE(Error, "{0}: Cannot sample stamp here, no query", this);
		return;
	}

	const int32 Num = ComputeVoxelBuffersNum(Position, PreviousDistance);

	VOXEL_FUNCTION_COUNTER_NUM(Num);
	FVoxelNodeStatScope StatScope(*this, Num);

	FVoxelDoubleVectorBuffer LocalPosition = Position;
	LocalPosition.ExpandConstants(Num);

	FVoxelSurfaceTypeBlendBuffer SurfaceTypes;
	if (SurfaceTypePin.ShouldCompute())
	{
		SurfaceTypes.AllocateZeroed(Num);
	}

	TVoxelMap<FVoxelMetadataRef, TSharedRef<FVoxelBuffer>> MetadataToBuffer;
	MetadataToBuffer.Reserve(MetadataPins.Num());

	for (const FMetadataPin& MetadataPin : MetadataPins)
	{
		if (MetadataPin.PinRef.ShouldCompute())
		{
			MetadataToBuffer.Add_EnsureNew(
				MetadataPin.MetadataRef,
				MetadataPin.MetadataRef.MakeDefaultBuffer(Num));
		}
	}

	FVoxelFloatBuffer Distance;
	Distance.Allocate(Num);
	Distance.CopyFrom(PreviousDistance, 0, 0, Num);

	const FVoxelQuery VoxelQuery(
		0,
		FVoxelLayers::Empty(),
		QueryParameter->Query.SurfaceTypeTable,
		Query->Context.DependencyCollector);

	const FVoxelVolumeSparseQuery SparseQuery = FVoxelVolumeSparseQuery::Create(
		VoxelQuery,
		Distance.View(),
		SurfaceTypes.View(),
		MetadataToBuffer,
		Position,
		SurfaceTypes.Num() > 0,
		MetadataToBuffer.KeyArray());

	StampRuntime->Apply(
		SparseQuery,
		{});

	DistancePin.Set(Query, Distance);

	if (SurfaceTypePin.ShouldCompute())
	{
		SurfaceTypePin.Set(Query, SurfaceTypes);
	}

	for (const FMetadataPin& MetadataPin : MetadataPins)
	{
		if (MetadataPin.PinRef.ShouldCompute())
		{
			MetadataPin.PinRef.Set(Query, FVoxelRuntimePinValue::Make(MetadataToBuffer[MetadataPin.MetadataRef]));
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
FString FVoxelNode_SampleHeightStamp::GetDisplayName() const
{
	if (!Stamp)
	{
		return "Sample Height Stamp";
	}

	FVoxelStamp::FActorLabelInfo Info;
	Stamp->GetActorLabelInfo(Info);

	if (!Info.Identifier.IsEmpty())
	{
		return "Sample Height Stamp: " + Info.Identifier;
	}

	FString Name = Stamp.GetStruct()->GetDisplayNameText().ToString();
	ensureVoxelSlow(Name.RemoveFromStart("Voxel "));
	ensureVoxelSlow(Name.RemoveFromEnd(" Stamp"));
	return "Sample Height Stamp: " + Name;
}
#endif

void FVoxelNode_SampleHeightStamp::Initialize(FInitializer& Initializer)
{
	VOXEL_FUNCTION_COUNTER();

	Super::Initialize(Initializer);

	const TSharedPtr<FVoxelStampRuntime> Runtime = FVoxelStampRuntime::Create(
		{},
		Stamp,
		{});

	StampRuntime = CastStructEnsured<FVoxelHeightStampRuntime>(Runtime);
}

void FVoxelNode_SampleHeightStamp::Compute(const FVoxelGraphQuery Query) const
{
	const TValue<FVoxelDoubleVector2DBuffer> Position = PositionPin.Get(Query);
	const TValue<FVoxelFloatBuffer> PreviousHeight = PreviousHeightPin.Get(Query);

	VOXEL_GRAPH_WAIT(Position, PreviousHeight)
	{
		ComputeImpl(
			Query,
			*Position,
			*PreviousHeight,
			StampRuntime);
	};
}

void FVoxelNode_SampleHeightStamp::ComputeNoCachePin(
	const FVoxelGraphQuery Query,
	const int32 PinIndex) const
{
	checkVoxelSlow(BoundsPin.GetPinIndex() == PinIndex);

	if (StampRuntime)
	{
		BoundsPin.Set(Query, StampRuntime->GetLocalBounds());
	}
	else
	{
		VOXEL_MESSAGE(Error, "{0}: Invalid stamp", this);
		BoundsPin.Set(Query, FVoxelBox2D());
	}
}

#if WITH_EDITOR
FString FVoxelNode_SampleVolumeStamp::GetDisplayName() const
{
	if (!Stamp)
	{
		return "Sample Volume Stamp";
	}

	FVoxelStamp::FActorLabelInfo Info;
	Stamp->GetActorLabelInfo(Info);

	if (!Info.Identifier.IsEmpty())
	{
		return "Sample Volume Stamp: " + Info.Identifier;
	}

	FString Name = Stamp.GetStruct()->GetDisplayNameText().ToString();
	ensureVoxelSlow(Name.RemoveFromStart("Voxel "));
	ensureVoxelSlow(Name.RemoveFromEnd(" Stamp"));
	return "Sample Volume Stamp: " + Name;
}
#endif

void FVoxelNode_SampleVolumeStamp::Initialize(FInitializer& Initializer)
{
	VOXEL_FUNCTION_COUNTER();

	Super::Initialize(Initializer);

	const TSharedPtr<FVoxelStampRuntime> Runtime = FVoxelStampRuntime::Create(
		{},
		Stamp,
		{});

	StampRuntime = CastStructEnsured<FVoxelVolumeStampRuntime>(Runtime);
}

void FVoxelNode_SampleVolumeStamp::Compute(const FVoxelGraphQuery Query) const
{
	const TValue<FVoxelDoubleVectorBuffer> Position = PositionPin.Get(Query);
	const TValue<FVoxelFloatBuffer> PreviousDistance = PreviousDistancePin.Get(Query);

	VOXEL_GRAPH_WAIT(Position, PreviousDistance)
	{
		ComputeImpl(
			Query,
			*Position,
			*PreviousDistance,
			StampRuntime);
	};
}

void FVoxelNode_SampleVolumeStamp::ComputeNoCachePin(
	const FVoxelGraphQuery Query,
	const int32 PinIndex) const
{
	checkVoxelSlow(BoundsPin.GetPinIndex() == PinIndex);

	if (StampRuntime)
	{
		BoundsPin.Set(Query, StampRuntime->GetLocalBounds());
	}
	else
	{
		VOXEL_MESSAGE(Error, "{0}: Invalid stamp", this);
		BoundsPin.Set(Query, FVoxelBox());
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNode_SampleHeightStampParameter::Compute(const FVoxelGraphQuery Query) const
{
	const TValue<FVoxelHeightGraphStampWrapper> Stamp = StampPin.Get(Query);
	const TValue<FVoxelDoubleVector2DBuffer> Position = PositionPin.Get(Query);
	const TValue<FVoxelFloatBuffer> PreviousHeight = PreviousHeightPin.Get(Query);

	VOXEL_GRAPH_WAIT(Stamp, Position, PreviousHeight)
	{
		ComputeImpl(
			Query,
			*Position,
			*PreviousHeight,
			Stamp->Stamp);
	};
}

void FVoxelNode_SampleHeightStampParameter::ComputeNoCachePin(
	const FVoxelGraphQuery Query,
	const int32 PinIndex) const
{
	checkVoxelSlow(BoundsPin.GetPinIndex() == PinIndex);

	const TValue<FVoxelHeightGraphStampWrapper> Stamp = StampPin.Get(Query);

	VOXEL_GRAPH_WAIT(Stamp)
	{
		if (Stamp->Stamp)
		{
			BoundsPin.Set(Query, Stamp->Stamp->GetLocalBounds());
		}
		else
		{
			VOXEL_MESSAGE(Error, "{0}: Invalid stamp", this);
			BoundsPin.Set(Query, FVoxelBox2D());
		}
	};
}

void FVoxelNode_SampleVolumeStampParameter::Compute(const FVoxelGraphQuery Query) const
{
	const TValue<FVoxelVolumeGraphStampWrapper> Stamp = StampPin.Get(Query);
	const TValue<FVoxelDoubleVectorBuffer> Position = PositionPin.Get(Query);
	const TValue<FVoxelFloatBuffer> PreviousDistance = PreviousDistancePin.Get(Query);

	VOXEL_GRAPH_WAIT(Stamp, Position, PreviousDistance)
	{
		ComputeImpl(
			Query,
			*Position,
			*PreviousDistance,
			Stamp->Stamp);
	};
}

void FVoxelNode_SampleVolumeStampParameter::ComputeNoCachePin(
	const FVoxelGraphQuery Query,
	const int32 PinIndex) const
{
	checkVoxelSlow(BoundsPin.GetPinIndex() == PinIndex);

	const TValue<FVoxelVolumeGraphStampWrapper> Stamp = StampPin.Get(Query);

	VOXEL_GRAPH_WAIT(Stamp)
	{
		if (Stamp->Stamp)
		{
			BoundsPin.Set(Query, Stamp->Stamp->GetLocalBounds());
		}
		else
		{
			VOXEL_MESSAGE(Error, "{0}: Invalid stamp", this);
			BoundsPin.Set(Query, FVoxelBox());
		}
	};
}