// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphEnvironment.h"
#include "VoxelGraph.h"
#include "VoxelExternalParameter.h"
#include "VoxelParameterView.h"
#include "VoxelParameterOverridesOwner.h"

bool FVoxelGraphEnvironment::operator==(const FVoxelGraphEnvironment& Other) const
{
	VOXEL_FUNCTION_COUNTER();

	return
		Owner == Other.Owner &&
		Component == Other.Component &&
		RootCompiledGraph == Other.RootCompiledGraph &&
		ParameterGuidToExposedValue.OrderIndependentEqual(Other.ParameterGuidToExposedValue) &&
		LocalToWorld.Equals(Other.LocalToWorld, 0.) &&
		LocalToWorld2D == Other.LocalToWorld2D &&
		bIsPreviewScene == Other.bIsPreviewScene;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<FVoxelGraphEnvironment> FVoxelGraphEnvironment::Create(
	const TVoxelObjectPtr<UObject> Owner,
	const TVoxelObjectPtr<USceneComponent> Component,
	const IVoxelParameterOverridesOwner& OverridesOwner,
	const FTransform& LocalToWorld,
	FVoxelDependencyCollector& CompiledGraphDependencyCollector)
{
	const UVoxelGraph* Graph = OverridesOwner.GetGraph();
	if (!Graph)
	{
		return {};
	}

	return CreateImpl(
		false,
		Owner,
		Component,
		*Graph,
		OverridesOwner.GetParameterOverrides(),
		LocalToWorld,
		CompiledGraphDependencyCollector);
}

TSharedRef<FVoxelGraphEnvironment> FVoxelGraphEnvironment::Create(
	const TVoxelObjectPtr<UObject> Owner,
	const TVoxelObjectPtr<USceneComponent> Component,
	const UVoxelGraph& Graph,
	const FVoxelParameterOverrides& Overrides,
	const FTransform& LocalToWorld,
	FVoxelDependencyCollector& CompiledGraphDependencyCollector)
{
	return CreateImpl(
		false,
		Owner,
		Component,
		Graph,
		Overrides,
		LocalToWorld,
		CompiledGraphDependencyCollector);
}

TSharedPtr<FVoxelGraphEnvironment> FVoxelGraphEnvironment::CreatePreview(
	const TVoxelObjectPtr<UObject> Owner,
	const TVoxelObjectPtr<USceneComponent> Component,
	const IVoxelParameterOverridesOwner& OverridesOwner,
	const FTransform& LocalToWorld,
	FVoxelDependencyCollector& CompiledGraphDependencyCollector)
{
	const UVoxelGraph* Graph = OverridesOwner.GetGraph();
	if (!Graph)
	{
		return {};
	}

	return CreateImpl(
		true,
		Owner,
		Component,
		*Graph,
		OverridesOwner.GetParameterOverrides(),
		LocalToWorld,
		CompiledGraphDependencyCollector);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelGraphEnvironment::FVoxelGraphEnvironment(
	const bool bIsPreviewScene,
	const TVoxelObjectPtr<UObject> Owner,
	const TVoxelObjectPtr<USceneComponent> Component,
	const TSharedRef<const FVoxelCompiledGraph>& RootCompiledGraph,
	TVoxelMap<FGuid, FVoxelPinValue>&& ParameterGuidToExposedValue,
	TVoxelMap<FGuid, FVoxelRuntimePinValue>&& ParameterGuidToValue,
	const FTransform& LocalToWorld)
	: Owner(Owner)
	, Component(Component)
	, RootCompiledGraph(RootCompiledGraph)
	, ParameterGuidToExposedValue(MoveTemp(ParameterGuidToExposedValue))
	, ParameterGuidToValue(MoveTemp(ParameterGuidToValue))
	, LocalToWorld(LocalToWorld)
	, LocalToWorld2D(FVoxelUtilities::MakeTransform2(LocalToWorld))
	, bIsPreviewScene(bIsPreviewScene)
{
}

TSharedRef<FVoxelGraphEnvironment> FVoxelGraphEnvironment::CreateImpl(
	const bool bIsPreviewScene,
	const TVoxelObjectPtr<UObject> Owner,
	const TVoxelObjectPtr<USceneComponent> Component,
	const UVoxelGraph& Graph,
	const FVoxelParameterOverrides& Overrides,
	const FTransform& LocalToWorld,
	FVoxelDependencyCollector& CompiledGraphDependencyCollector)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	const TSharedRef<const FVoxelCompiledGraph> CompiledGraph = Graph.GetCompiledGraph(CompiledGraphDependencyCollector);

	TVoxelArray<const FVoxelParameterOverrides*> AllParameterOverrides;
	{
		AllParameterOverrides.Reserve(16);
		AllParameterOverrides.Add(&Overrides);

		for (const UVoxelGraph* BaseGraph : Graph.GetBaseGraphs())
		{
			// If OverridesOwner is a graph it'll be added twice but that's fine
			AllParameterOverrides.Add(&ConstCast(BaseGraph)->GetParameterOverrides());
		}
	}

	TVoxelMap<FGuid, FVoxelPinValue> NewParameterGuidToExposedValue;
	TVoxelMap<FGuid, FVoxelRuntimePinValue> NewParameterGuidToValue;
	{
		VOXEL_SCOPE_COUNTER_NUM("Build parameters", Graph.NumParameters());

		NewParameterGuidToExposedValue.Reserve(Graph.NumParameters());
		NewParameterGuidToValue.Reserve(Graph.NumParameters());

		const FVoxelPinType::FRuntimeValueContext RuntimeValueContext
		{
			Owner,
			Component
		};

		Graph.ForeachParameter([&](const FGuid& Guid, const FVoxelParameter& Parameter)
		{
			FVoxelPinValue Value = INLINE_LAMBDA
			{
				for (const FVoxelParameterOverrides* ParameterOverrides : AllParameterOverrides)
				{
					const FVoxelParameterValueOverride* Override = ParameterOverrides->GuidToValueOverride.Find(Guid);
					if (!Override ||
						!Override->bEnable)
					{
						continue;
					}

					return Override->Value;
				}

				return FVoxelPinValue(Parameter.Type.GetExposedType());
			};

			FVoxelRuntimePinValue RuntimeValue = FVoxelPinType::MakeRuntimeValue(
				Parameter.Type,
				Value,
				RuntimeValueContext);

			ensure(RuntimeValue.IsValid());

			if (RuntimeValue.GetType().CanBeCastedTo<FVoxelExternalParameter>())
			{
				ConstCast(RuntimeValue.Get<FVoxelExternalParameter>()).ParameterGuid = Guid;
			}

			NewParameterGuidToExposedValue.Add_EnsureNew(Guid, MoveTemp(Value));
			NewParameterGuidToValue.Add_EnsureNew(Guid, MoveTemp(RuntimeValue));
		});
	}

	return MakeShareable(new FVoxelGraphEnvironment(
		bIsPreviewScene,
		Owner,
		Component,
		CompiledGraph,
		MoveTemp(NewParameterGuidToExposedValue),
		MoveTemp(NewParameterGuidToValue),
		LocalToWorld));
}