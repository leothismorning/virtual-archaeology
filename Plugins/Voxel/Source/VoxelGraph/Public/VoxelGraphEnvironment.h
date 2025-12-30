// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelGraphParameters.h"
#include "VoxelRuntimePinValue.h"

class UVoxelGraph;
class UVoxelTerminalGraph;
class FVoxelCompiledGraph;
class IVoxelParameterOverridesOwner;
struct FVoxelParameterOverrides;

class VOXELGRAPH_API FVoxelGraphEnvironment : public TSharedFromThis<FVoxelGraphEnvironment>
{
public:
	const TVoxelObjectPtr<UObject> Owner;
	const TVoxelObjectPtr<USceneComponent> Component;
	const TSharedRef<const FVoxelCompiledGraph> RootCompiledGraph;
	// Used to detect changes
	const TVoxelMap<FGuid, FVoxelPinValue> ParameterGuidToExposedValue;
	const TVoxelMap<FGuid, FVoxelRuntimePinValue> ParameterGuidToValue;
	const FTransform LocalToWorld;
	const FTransform2d LocalToWorld2D;
	const bool bIsPreviewScene;

	bool operator==(const FVoxelGraphEnvironment& Other) const;

public:
	static TSharedPtr<FVoxelGraphEnvironment> Create(
		TVoxelObjectPtr<UObject> Owner,
		TVoxelObjectPtr<USceneComponent> Component,
		const IVoxelParameterOverridesOwner& OverridesOwner,
		const FTransform& LocalToWorld,
		FVoxelDependencyCollector& CompiledGraphDependencyCollector);

	static TSharedRef<FVoxelGraphEnvironment> Create(
		TVoxelObjectPtr<UObject> Owner,
		TVoxelObjectPtr<USceneComponent> Component,
		const UVoxelGraph& Graph,
		const FVoxelParameterOverrides& Overrides,
		const FTransform& LocalToWorld,
		FVoxelDependencyCollector& CompiledGraphDependencyCollector);

	static TSharedPtr<FVoxelGraphEnvironment> CreatePreview(
		TVoxelObjectPtr<UObject> Owner,
		TVoxelObjectPtr<USceneComponent> Component,
		const IVoxelParameterOverridesOwner& OverridesOwner,
		const FTransform& LocalToWorld,
		FVoxelDependencyCollector& CompiledGraphDependencyCollector);

private:
	FVoxelGraphEnvironment(
		bool bIsPreviewScene,
		TVoxelObjectPtr<UObject> Owner,
		TVoxelObjectPtr<USceneComponent> Component,
		const TSharedRef<const FVoxelCompiledGraph>& RootCompiledGraph,
		TVoxelMap<FGuid, FVoxelPinValue>&& ParameterGuidToExposedValue,
		TVoxelMap<FGuid, FVoxelRuntimePinValue>&& ParameterGuidToValue,
		const FTransform& LocalToWorld);

	TVoxelArray<TSharedRef<FVoxelGraphParameters::FUniformParameter>> UniformParameters;
	TVoxelArray<const FVoxelGraphParameters::FUniformParameter*> DefaultNameIndexToUniformParameter;

	static TSharedRef<FVoxelGraphEnvironment> CreateImpl(
		bool bIsPreviewScene,
		TVoxelObjectPtr<UObject> Owner,
		TVoxelObjectPtr<USceneComponent> Component,
		const UVoxelGraph& Graph,
		const FVoxelParameterOverrides& Overrides,
		const FTransform& LocalToWorld,
		FVoxelDependencyCollector& CompiledGraphDependencyCollector);

	friend class FVoxelGraphContext;

public:
	template<typename T, typename... ArgTypes>
	requires
	(
		FVoxelGraphParameters::IsUniform<T> &&
		std::is_constructible_v<T, ArgTypes&&...>
	)
	T& AddParameter(ArgTypes&&... Args)
	{
		const TSharedRef<T> Parameter = MakeShared<T>(Forward<ArgTypes>(Args)...);
		UniformParameters.Add(Parameter);

		const int32 ParameterIndex = FVoxelGraphParameterManager::GetUniformIndex<T>();

		if (DefaultNameIndexToUniformParameter.Num() <= ParameterIndex)
		{
			DefaultNameIndexToUniformParameter.SetNumZeroed(FVoxelGraphParameterManager::NumUniform());
		}
		DefaultNameIndexToUniformParameter[ParameterIndex] = &Parameter.Get();

		return *Parameter;
	}
};