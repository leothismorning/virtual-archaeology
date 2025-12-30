// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Graphs/VoxelSamplePreviousStampsNodes.h"
#include "Graphs/VoxelStampGraphParameters.h"
#include "VoxelQuery.h"
#include "VoxelMetadata.h"
#include "Surface/VoxelSmartSurfaceFunctionLibrary.h"
#include "VoxelSamplePreviousStampsNodesImpl.ispc.generated.h"

FVoxelNode_SamplePreviousStampsBase::FVoxelNode_SamplePreviousStampsBase()
{
	FixupMetadataPins();
}

void FVoxelNode_SamplePreviousStampsBase::Initialize(FInitializer& Initializer)
{
	VOXEL_FUNCTION_COUNTER();

	for (FMetadataPin& MetadataPin : MetadataPins)
	{
		Initializer.InitializePinRef(MetadataPin.PinRef);
	}
}

void FVoxelNode_SamplePreviousStampsBase::PostSerialize()
{
	Super::PostSerialize();

	FixupMetadataPins();
}

#if WITH_EDITOR
FVoxelNode::EPostEditChange FVoxelNode_SamplePreviousStampsBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.GetPropertyName() == GET_OWN_MEMBER_NAME(MetadatasToQuery))
	{
		FixupMetadataPins();
		return EPostEditChange::Reconstruct;
	}

	return EPostEditChange::None;
}
#endif

void FVoxelNode_SamplePreviousStampsBase::FixupMetadataPins()
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

void FVoxelNode_SamplePreviousHeightStamps::Compute(const FVoxelGraphQuery Query) const
{
	const TValue<FVoxelDoubleVector2DBuffer> InPosition = PositionPin.Get(Query);

	VOXEL_GRAPH_WAIT(InPosition)
	{
		FVoxelDoubleVector2DBuffer Position = *InPosition;
		Position.ExpandConstants(Position.Num());

		VOXEL_FUNCTION_COUNTER_NUM(Position.Num());
		FVoxelNodeStatScope StatScope(*this, Position.Num());

		const FVoxelGraphParameters::FHeightStamp* HeightStamp = Query->FindParameter<FVoxelGraphParameters::FHeightStamp>();
		if (!HeightStamp)
		{
			VOXEL_MESSAGE(Error, "{0}: not in a height stamp", this);
			return;
		}
		if (HeightStamp->BlendMode != EVoxelHeightBlendMode::Override)
		{
			VOXEL_MESSAGE(Error, "{0}: blend mode should be set to Override", this);
			return;
		}

		const FVoxelGraphParameters::FHeightQuery* HeightQuery = Query->FindParameter<FVoxelGraphParameters::FHeightQuery>();
		if (!HeightQuery ||
			!HeightQuery->QueryPrevious)
		{
			VOXEL_MESSAGE(Error, "{0}: no previous stamp data", this);
			return;
		}

		const FVoxelGraphParameters::FHeightQueryPrevious* HeightQueryPrevious = Query->FindParameter<FVoxelGraphParameters::FHeightQueryPrevious>();

		// Will be null in bulk queries
		if (HeightQueryPrevious)
		{
			if (HeightQueryPrevious->bQueried ||
				HeightQueryPrevious->GlobalPositions.X.GetData() != Position.X.GetData() ||
				HeightQueryPrevious->GlobalPositions.Y.GetData() != Position.Y.GetData())
			{
				HeightQueryPrevious = nullptr;
			}
		}

		VOXEL_SCOPE_COUNTER_COND(HeightQueryPrevious, "Query global previous");

		FVoxelSurfaceTypeBlendBuffer SurfaceTypes;
		if (SurfaceTypePin.ShouldCompute() ||
			(HeightQueryPrevious && HeightQueryPrevious->bQueryPreviousSurfaceTypes))
		{
			SurfaceTypes.AllocateZeroed(Position.Num());
		}

		TVoxelMap<FVoxelMetadataRef, TSharedRef<FVoxelBuffer>> MetadataToBuffer;
		MetadataToBuffer.Reserve(MetadataPins.Num());

		for (const FMetadataPin& MetadataPin : MetadataPins)
		{
			if (MetadataPin.PinRef.ShouldCompute())
			{
				MetadataToBuffer.Add_EnsureNew(
					MetadataPin.MetadataRef,
					MetadataPin.MetadataRef.MakeDefaultBuffer(Position.Num()));
			}
		}

		if (HeightQueryPrevious)
		{
			for (const FVoxelMetadataRef& Metadata : HeightQueryPrevious->PreviousMetadatasToQuery)
			{
				if (MetadataToBuffer.Contains(Metadata))
				{
					continue;
				}

				MetadataToBuffer.Add_EnsureNew(
					Metadata,
					Metadata.MakeDefaultBuffer(Position.Num()));
			}
		}

		FVoxelFloatBuffer Height;
		Height.Allocate(Position.Num());
		Height.SetAll(FVoxelUtilities::NaNf());

		FVoxelDoubleVector2DBuffer QueryPosition;
		{
			VOXEL_SCOPE_COUNTER_NUM("VoxelSamplePreviousStampsNodes_TransformHeightPositions", Position.Num());

			QueryPosition.Allocate(Position.Num());

			ispc::VoxelSamplePreviousStampsNodes_TransformHeightPositions(
				HeightQuery->StampToQuery.ISPC(),
				Position.X.GetData(),
				Position.Y.GetData(),
				QueryPosition.X.GetData(),
				QueryPosition.Y.GetData(),
				Position.Num());
		}

		HeightQuery->QueryPrevious->Query(FVoxelHeightSparseQuery::Create(
			HeightQuery->Query,
			Height.View(),
			SurfaceTypes.View(),
			MetadataToBuffer,
			QueryPosition,
			SurfaceTypes.Num() > 0,
			MetadataToBuffer.KeyArray()));

		if (HeightQueryPrevious)
		{
			ensure(!HeightQueryPrevious->bQueried);
			HeightQueryPrevious->bQueried = true;

			if (HeightQueryPrevious->bQueryPreviousValues)
			{
				HeightQueryPrevious->OutValues = HeightPin.ShouldCompute() ? Height.MakeDeepCopy() : Height;
			}

			if (HeightQueryPrevious->bQueryPreviousSurfaceTypes)
			{
				HeightQueryPrevious->OutSurfaceTypes = SurfaceTypes;
			}

			HeightQueryPrevious->OutMetadataToBuffer.Reserve(HeightQueryPrevious->PreviousMetadatasToQuery.Num());

			for (const FVoxelMetadataRef& Metadata : HeightQueryPrevious->PreviousMetadatasToQuery)
			{
				HeightQueryPrevious->OutMetadataToBuffer.Add_EnsureNew(Metadata, MetadataToBuffer[Metadata]);
			}
		}

		if (HeightPin.ShouldCompute())
		{
			VOXEL_SCOPE_COUNTER_NUM("VoxelSamplePreviousStampsNodes_TransformHeights", Height.Num());

			ispc::VoxelSamplePreviousStampsNodes_TransformHeights(
				HeightQuery->StampToQuery.ISPC(),
				Position.X.GetData(),
				Position.Y.GetData(),
				Height.GetData(),
				Height.Num());

			HeightPin.Set(Query, Height);
		}

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
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNode_SamplePreviousVolumeStamps::Compute(const FVoxelGraphQuery Query) const
{
	const TValue<FVoxelDoubleVectorBuffer> InPosition = PositionPin.Get(Query);

	VOXEL_GRAPH_WAIT(InPosition)
	{
		FVoxelDoubleVectorBuffer Position = *InPosition;
		Position.ExpandConstants(Position.Num());

		VOXEL_FUNCTION_COUNTER_NUM(Position.Num());
		FVoxelNodeStatScope StatScope(*this, Position.Num());

		const FVoxelGraphParameters::FVolumeStamp* VolumeStamp = Query->FindParameter<FVoxelGraphParameters::FVolumeStamp>();
		if (!VolumeStamp)
		{
			VOXEL_MESSAGE(Error, "{0}: not in a volume stamp", this);
			return;
		}
		if (VolumeStamp->BlendMode != EVoxelVolumeBlendMode::Override)
		{
			VOXEL_MESSAGE(Error, "{0}: blend mode should be set to Override", this);
			return;
		}

		const FVoxelGraphParameters::FVolumeQuery* VolumeQuery = Query->FindParameter<FVoxelGraphParameters::FVolumeQuery>();
		if (!VolumeQuery ||
			!VolumeQuery->QueryPrevious)
		{
			VOXEL_MESSAGE(Error, "{0}: no previous stamp data", this);
			return;
		}

		const FVoxelGraphParameters::FVolumeQueryPrevious* VolumeQueryPrevious = Query->FindParameter<FVoxelGraphParameters::FVolumeQueryPrevious>();

		// Will be null in bulk queries
		if (VolumeQueryPrevious)
		{
			if (VolumeQueryPrevious->bQueried ||
				VolumeQueryPrevious->GlobalPositions.X.GetData() != Position.X.GetData() ||
				VolumeQueryPrevious->GlobalPositions.Y.GetData() != Position.Y.GetData() ||
				VolumeQueryPrevious->GlobalPositions.Z.GetData() != Position.Z.GetData())
			{
				VolumeQueryPrevious = nullptr;
			}
		}

		VOXEL_SCOPE_COUNTER_COND(VolumeQueryPrevious, "Query global previous");

		FVoxelSurfaceTypeBlendBuffer SurfaceTypes;
		if (SurfaceTypePin.ShouldCompute() ||
			(VolumeQueryPrevious && VolumeQueryPrevious->bQueryPreviousSurfaceTypes))
		{
			SurfaceTypes.AllocateZeroed(Position.Num());
		}

		TVoxelMap<FVoxelMetadataRef, TSharedRef<FVoxelBuffer>> MetadataToBuffer;
		MetadataToBuffer.Reserve(MetadataPins.Num());

		for (const FMetadataPin& MetadataPin : MetadataPins)
		{
			if (MetadataPin.PinRef.ShouldCompute())
			{
				MetadataToBuffer.Add_EnsureNew(
					MetadataPin.MetadataRef,
					MetadataPin.MetadataRef.MakeDefaultBuffer(Position.Num()));
			}
		}

		if (VolumeQueryPrevious)
		{
			for (const FVoxelMetadataRef& Metadata : VolumeQueryPrevious->PreviousMetadatasToQuery)
			{
				if (MetadataToBuffer.Contains(Metadata))
				{
					continue;
				}

				MetadataToBuffer.Add_EnsureNew(
					Metadata,
					Metadata.MakeDefaultBuffer(Position.Num()));
			}
		}

		FVoxelFloatBuffer Distance;
		Distance.Allocate(Position.Num());
		Distance.SetAll(FVoxelUtilities::NaNf());

		FVoxelDoubleVectorBuffer QueryPosition;
		{
			VOXEL_SCOPE_COUNTER_NUM("VoxelSamplePreviousStampsNodes_TransformVolumePositions", Position.Num());

			QueryPosition.Allocate(Position.Num());

			ispc::VoxelSamplePreviousStampsNodes_TransformVolumePositions(
				VolumeQuery->StampToQuery.ISPC(),
				Position.X.GetData(),
				Position.Y.GetData(),
				Position.Z.GetData(),
				QueryPosition.X.GetData(),
				QueryPosition.Y.GetData(),
				QueryPosition.Z.GetData(),
				Position.Num());
		}

		VolumeQuery->QueryPrevious->Query(FVoxelVolumeSparseQuery::Create(
			VolumeQuery->Query,
			Distance.View(),
			SurfaceTypes.View(),
			MetadataToBuffer,
			QueryPosition,
			SurfaceTypes.Num() > 0,
			MetadataToBuffer.KeyArray()));

		if (VolumeQueryPrevious)
		{
			ensure(!VolumeQueryPrevious->bQueried);
			VolumeQueryPrevious->bQueried = true;

			if (VolumeQueryPrevious->bQueryPreviousValues)
			{
				VolumeQueryPrevious->OutValues = DistancePin.ShouldCompute() ? Distance.MakeDeepCopy() : Distance;
			}

			if (VolumeQueryPrevious->bQueryPreviousSurfaceTypes)
			{
				VolumeQueryPrevious->OutSurfaceTypes = SurfaceTypes;
			}

			VolumeQueryPrevious->OutMetadataToBuffer.Reserve(VolumeQueryPrevious->PreviousMetadatasToQuery.Num());

			for (const FVoxelMetadataRef& Metadata : VolumeQueryPrevious->PreviousMetadatasToQuery)
			{
				VolumeQueryPrevious->OutMetadataToBuffer.Add_EnsureNew(Metadata, MetadataToBuffer[Metadata]);
			}
		}

		if (DistancePin.ShouldCompute())
		{
			VOXEL_SCOPE_COUNTER_NUM("VoxelSamplePreviousStampsNodes_TransformDistances", Distance.Num());

			ispc::VoxelSamplePreviousStampsNodes_TransformDistances(
				VolumeQuery->StampToQuery.ISPC(),
				Distance.GetData(),
				Distance.Num());

			DistancePin.Set(Query, Distance);
		}

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
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNode_SampleTerrain::Compute(const FVoxelGraphQuery Query) const
{
	const TValue<FVoxelDoubleVectorBuffer> InPosition = PositionPin.Get(Query);

	VOXEL_GRAPH_WAIT(InPosition)
	{
		FVoxelDoubleVectorBuffer Position = *InPosition;
		Position.ExpandConstants(Position.Num());

		VOXEL_FUNCTION_COUNTER_NUM(Position.Num());
		FVoxelNodeStatScope StatScope(*this, Position.Num());

		const FVoxelGraphParameters::FSmartSurfaceUniform* SmartSurface = Query->FindParameter<FVoxelGraphParameters::FSmartSurfaceUniform>();
		if (!SmartSurface)
		{
			VOXEL_MESSAGE(Error, "{0}: no previous stamp data, can only be used in surface graphs", this);
			return;
		}

		const FVoxelGraphParameters::FQuery* QueryParameter = Query->FindParameter<FVoxelGraphParameters::FQuery>();
		if (!ensureVoxelSlow(QueryParameter))
		{
			VOXEL_MESSAGE(Error, "{0}: no previous stamp data", this);
			return;
		}

		FVoxelSurfaceTypeBlendBuffer SurfaceTypes;
		if (SurfaceTypePin.ShouldCompute())
		{
			SurfaceTypes.AllocateZeroed(Position.Num());
		}

		TVoxelMap<FVoxelMetadataRef, TSharedRef<FVoxelBuffer>> MetadataToBuffer;
		MetadataToBuffer.Reserve(MetadataPins.Num());

		for (const FMetadataPin& MetadataPin : MetadataPins)
		{
			if (MetadataPin.PinRef.ShouldCompute())
			{
				MetadataToBuffer.Add_EnsureNew(
					MetadataPin.MetadataRef,
					MetadataPin.MetadataRef.MakeDefaultBuffer(Position.Num()));
			}
		}

		const FVoxelFloatBuffer Distance = QueryParameter->Query.SampleVolumeLayer(
			SmartSurface->WeakLayer,
			Position,
			SurfaceTypes.View(),
			MetadataToBuffer);

		if (DistancePin.ShouldCompute())
		{
			DistancePin.Set(Query, Distance);
		}

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
	};
}