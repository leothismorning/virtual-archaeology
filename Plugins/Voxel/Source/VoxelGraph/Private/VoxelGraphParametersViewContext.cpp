// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphParametersViewContext.h"
#include "VoxelParameterOverridesOwner.h"
#include "VoxelGraph.h"

const FVoxelParameterValueOverride* FVoxelGraphParametersViewContext::FindValueOverride(const FGuid& Guid) const
{
	VOXEL_FUNCTION_COUNTER();

	// First check the main overrides
	// This is typically a voxel actor or a graph
	const FVoxelParameterValueOverride* MainValueOverride = INLINE_LAMBDA -> const FVoxelParameterValueOverride*
	{
		if (bSkipMainOverridesOwner)
		{
			return nullptr;
		}

		const IVoxelParameterOverridesOwner* OverridesOwner = MainOverridesOwner.Get();
		if (!ensure(OverridesOwner))
		{
			return nullptr;
		}

		const FVoxelParameterValueOverride* ValueOverride = OverridesOwner->GetGuidToValueOverride().Find(Guid);
		if (!ValueOverride)
		{
			return nullptr;
		}

		if (!bForceEnableMainOverrides &&
			!ValueOverride->bEnable)
		{
			return nullptr;
		}

		return ValueOverride;
	};
	if (MainValueOverride)
	{
		return MainValueOverride;
	}

	for (const TVoxelObjectPtr<const UVoxelGraph>& WeakGraph : Graphs)
	{
		const UVoxelGraph* Graph = WeakGraph.Resolve();
		if (!ensure(Graph))
		{
			continue;
		}

		const FVoxelParameterValueOverride* ValueOverride = Graph->GetGuidToValueOverride().Find(Guid);
		if (!ValueOverride ||
			!ValueOverride->bEnable)
		{
			continue;
		}

		return ValueOverride;
	}

	return nullptr;
}