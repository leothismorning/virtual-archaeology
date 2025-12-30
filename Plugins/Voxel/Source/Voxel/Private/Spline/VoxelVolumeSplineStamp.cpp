// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Spline/VoxelVolumeSplineStamp.h"
#include "Spline/VoxelVolumeSplineGraph.h"
#include "Spline/VoxelSplineMetadata.h"
#include "Spline/VoxelSplineComponent.h"
#include "Spline/VoxelSplineGraphParameters.h"
#include "Spline/VoxelOutputNode_OutputVolumeSpline.h"
#include "VoxelStampRef.h"
#include "VoxelGraphTracker.h"
#include "VoxelGraphPositionParameter.h"
#include "VoxelQuery.h"
#include "VoxelTerminalGraph.h"
#include "VoxelStampUtilities.h"
#include "Graphs/VoxelStampGraphParameters.h"

TSharedPtr<FVoxelGraphEnvironment> FVoxelVolumeSplineStamp::CreateEnvironment(
	const FVoxelStampRuntime& Runtime,
	FVoxelDependencyCollector& DependencyCollector) const
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedPtr<FVoxelGraphEnvironment> Environment = FVoxelGraphEnvironment::Create(
		Runtime.GetComponent(),
		Runtime.GetComponent(),
		*this,
		Runtime.GetLocalToWorld(),
		DependencyCollector);

	if (!Environment)
	{
		return nullptr;
	}

	FVoxelGraphParameters::FVolumeStamp& Parameter = Environment->AddParameter<FVoxelGraphParameters::FVolumeStamp>();
	Parameter.Smoothness = Smoothness;
	Parameter.BlendMode = BlendMode;

	return Environment;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<IVoxelParameterOverridesOwner> FVoxelVolumeSplineStamp::GetShared() const
{
	return SharedThis(ConstCast(this));
}

bool FVoxelVolumeSplineStamp::ShouldForceEnableOverride(const FGuid& Guid) const
{
	return true;
}

UVoxelGraph* FVoxelVolumeSplineStamp::GetGraph() const
{
	return Graph;
}

FVoxelParameterOverrides& FVoxelVolumeSplineStamp::GetParameterOverrides()
{
	return ParameterOverrides;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelVolumeSplineStamp::FixupProperties(const IVoxelStampComponentInterface* Interface)
{
	Super::FixupProperties(Interface);

	FixupParameterOverrides();

	if (!Interface ||
		!Graph)
	{
		return;
	}

	const UVoxelSplineComponent* Component = Interface->FindComponent<UVoxelSplineComponent>();
	if (!Component ||
		!ensure(Component->Metadata))
	{
		return;
	}

	Component->Metadata->Fixup(*Graph);
}

TVoxelArray<TSubclassOf<USceneComponent>> FVoxelVolumeSplineStamp::GetRequiredComponents() const
{
	return { UVoxelSplineComponent::StaticClass() };
}

#if WITH_EDITOR
void FVoxelVolumeSplineStamp::GetActorLabelInfo(FActorLabelInfo& Info) const
{
	Info.Identifier = Graph ? Graph->GetMetadata().DisplayName : "None";
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelVolumeSplineStampRuntime::Initialize(FVoxelDependencyCollector& DependencyCollector)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	if (!Stamp.Graph)
	{
		return false;
	}

	const TSharedPtr<const FVoxelGraphEnvironment> Environment = Stamp.CreateEnvironment(
		*this,
		DependencyCollector);

	if (!Environment)
	{
		return false;
	}

	Evaluator = FVoxelNodeEvaluator::Create<FVoxelOutputNode_OutputVolumeSpline>(Environment.ToSharedRef());

	if (!Evaluator)
	{
		return false;
	}

#if WITH_EDITOR
	GVoxelGraphTracker->OnParameterValueChanged(*Stamp.Graph).Add(FOnVoxelGraphChanged::Make(this, [this]
	{
		RequestUpdate();
	}));
#endif

	const UVoxelSplineComponent* Component = FindComponent<UVoxelSplineComponent>();
	if (!Component ||
		Component->GetNumberOfSplinePoints() <= 1 ||
		!ensure(Component->Metadata))
	{
		// NumPoints = 1 is annoying to deal with
		return false;
	}

	MetadataRuntime = Component->Metadata->GetRuntime();

	bClosedLoop = Component->IsClosedLoop();
	ReparamStepsPerSegment = Component->ReparamStepsPerSegment;
	SplineLength = Component->GetSplineLength();
	SplineCurves = Component->SplineCurves;

	Segments = FVoxelSplineSegment::Create(SplineCurves, *MetadataRuntime);

	return true;
}

bool FVoxelVolumeSplineStampRuntime::Initialize_Parallel(FVoxelDependencyCollector& DependencyCollector)
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelGraphContext Context = Evaluator.MakeContext(DependencyCollector);
	FVoxelGraphQueryImpl& GraphQuery = Context.MakeQuery();

	MaxWidth = Evaluator->MaxWidthPin.GetSynchronous(GraphQuery);

	MetadataOverrides = Stamp.MetadataOverrides.CreateRuntime(
		GraphQuery,
		Evaluator);

	ensure(
		SplineCurves.Position.LoopKeyOffset == 0.f ||
		SplineCurves.Position.LoopKeyOffset == 1.f);

	for (int32 Index = 0; Index < SplineCurves.Position.Points.Num(); Index++)
	{
		ensure(SplineCurves.Position.Points[Index].InVal == Index);
		ensure(SplineCurves.Rotation.Points[Index].InVal == Index);
		ensure(SplineCurves.Scale.Points[Index].InVal == Index);

		ensure(SplineCurves.Position.Points[Index].InVal == Index);
		ensure(SplineCurves.Rotation.Points[Index].InVal == Index);
		ensure(SplineCurves.Scale.Points[Index].InVal == Index);
	}

	FVoxelVolumeSplineStamp& MutableStamp = GetMutableStamp();
	MutableStamp.bDisableEditingLayers = Evaluator->EnableLayerOverridePin.GetSynchronous(GraphQuery);
	MutableStamp.bDisableEditingBlendMode = Evaluator->EnableBlendModeOverridePin.GetSynchronous(GraphQuery);

	if (MutableStamp.bDisableEditingLayers)
	{
		MutableStamp.Layer = Evaluator->LayerOverridePin.GetSynchronous(GraphQuery).Layer.Resolve_Ensured();
		MutableStamp.AdditionalLayers = {};
	}

	if (MutableStamp.bDisableEditingBlendMode)
	{
		MutableStamp.BlendMode = Evaluator->BlendModeOverridePin.GetSynchronous(GraphQuery);
	}

	for (FVoxelSplineSegment& Segment : Segments)
	{
		Segment.Bounds = Segment.Bounds.Extend(MaxWidth);
	}

	return true;
}

FVoxelBox FVoxelVolumeSplineStampRuntime::GetLocalBounds() const
{
	return FVoxelBox::FromBounds(GetChildren());
}

bool FVoxelVolumeSplineStampRuntime::ShouldUseQueryPrevious() const
{
	return GetBlendMode() == EVoxelVolumeBlendMode::Override;
}

TVoxelInlineArray<FVoxelBox, 1> FVoxelVolumeSplineStampRuntime::GetChildren() const
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelInlineArray<FVoxelBox, 1> Result;
	Result.Reserve(Segments.Num());

	for (const FVoxelSplineSegment& Segment : Segments)
	{
		Result.Add_EnsureNoGrow(Segment.Bounds);
	}

	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelVolumeSplineStampRuntime::ShouldFullyInvalidate(
	const FVoxelStampRuntime& PreviousRuntime,
	TVoxelArray<FVoxelBox>& OutLocalBoundsToInvalidate) const
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelVolumeSplineStampRuntime& Other = CastStructChecked<FVoxelVolumeSplineStampRuntime>(PreviousRuntime);

	if (Evaluator != Other.Evaluator ||
		MaxWidth != Other.MaxWidth ||
		*MetadataOverrides != *Other.MetadataOverrides)
	{
		return true;
	}

	const TVoxelSet<FVoxelSplineSegment> OldSegments(Other.Segments);
	const TVoxelSet<FVoxelSplineSegment> NewSegments(Segments);

	for (const FVoxelSplineSegment& OldSegment : OldSegments)
	{
		if (!NewSegments.Contains(OldSegment))
		{
			OutLocalBoundsToInvalidate.Add(OldSegment.Bounds);
		}
	}

	for (const FVoxelSplineSegment& NewSegment : NewSegments)
	{
		if (!OldSegments.Contains(NewSegment))
		{
			OutLocalBoundsToInvalidate.Add(NewSegment.Bounds);
		}
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelVolumeSplineStampRuntime::Apply(
	const FVoxelVolumeBulkQuery& Query,
	const FVoxelVolumeTransform& StampToQuery) const
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelDoubleVectorBuffer Positions = FVoxelStampUtilities::ComputePositions(
		Query,
		StampToQuery);

	FVoxelGraphContext Context = Evaluator.MakeContext(Query.Query.DependencyCollector);

	FVoxelGraphQueryImpl& GraphQuery = Context.MakeQuery();
	GraphQuery.AddParameter<FVoxelGraphParameters::FLOD>().Value = Query.Query.LOD;
	GraphQuery.AddParameter<FVoxelGraphParameters::FQuery>(Query.Query);
	GraphQuery.AddParameter<FVoxelGraphParameters::FPosition3D>().SetLocalPosition(Positions);
	GraphQuery.AddParameter<FVoxelGraphParameters::FVolumeQuery>(Query, StampToQuery);

	{
		FVoxelGraphParameters::FSplineStamp& Parameter = GraphQuery.AddParameter<FVoxelGraphParameters::FSplineStamp>();
		Parameter.bIs2D = false;
		Parameter.bClosedLoop = bClosedLoop;
		Parameter.SplineLength = SplineLength;
		Parameter.ReparamStepsPerSegment = ReparamStepsPerSegment;
		Parameter.SplineCurves = &SplineCurves;
		Parameter.MetadataRuntime = MetadataRuntime.Get();
		Parameter.Segments = Segments;
	}

	FVoxelStampUtilities::ApplyDistances(
		*this,
		Query,
		StampToQuery,
		*Evaluator->DistancePin.GetSynchronous(GraphQuery));
}

void FVoxelVolumeSplineStampRuntime::Apply(
	const FVoxelVolumeSparseQuery& Query,
	const FVoxelVolumeTransform& StampToQuery) const
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelDoubleVectorBuffer Positions = FVoxelStampUtilities::ComputePositions(
		Query,
		StampToQuery);

	FVoxelGraphContext Context = Evaluator.MakeContext(Query.Query.DependencyCollector);

	FVoxelGraphQueryImpl& GraphQuery = Context.MakeQuery();
	GraphQuery.AddParameter<FVoxelGraphParameters::FLOD>().Value = Query.Query.LOD;
	GraphQuery.AddParameter<FVoxelGraphParameters::FQuery>(Query.Query);
	GraphQuery.AddParameter<FVoxelGraphParameters::FPosition3D>().SetLocalPosition(Positions);
	GraphQuery.AddParameter<FVoxelGraphParameters::FVolumeQuery>(Query, StampToQuery);

	{
		FVoxelGraphParameters::FSplineStamp& Parameter = GraphQuery.AddParameter<FVoxelGraphParameters::FSplineStamp>();
		Parameter.bIs2D = false;
		Parameter.bClosedLoop = bClosedLoop;
		Parameter.SplineLength = SplineLength;
		Parameter.ReparamStepsPerSegment = ReparamStepsPerSegment;
		Parameter.SplineCurves = &SplineCurves;
		Parameter.MetadataRuntime = MetadataRuntime.Get();
		Parameter.Segments = Segments;
	}

	FVoxelStampUtilities::ComputeVolumeStamp(
		*this,
		Query,
		StampToQuery,
		*MetadataOverrides,
		GraphQuery,
		Evaluator,
		Positions);
}