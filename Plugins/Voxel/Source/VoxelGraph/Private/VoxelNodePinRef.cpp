// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelNodePinRef.h"
#include "VoxelNode.h"

void FVoxelNode::FPinRef_Input::ComputeLinkedNode(const FVoxelGraphQuery Query) const
{
	if (LinkedPinMetadata.bNoCache)
	{
		LinkedNode->ComputeNoCachePin(Query, LinkedPinIndex);
	}
	else
	{
		LinkedNode->ComputeIfNeeded(Query, LinkedPinIndex);
	}
}

FVoxelRuntimePinValue FVoxelNode::FPinRef_Input::GetSynchronous(const FVoxelGraphQueryImpl& Query) const
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_COUNTER_FNAME(Name);

	FVoxelGraphCallstack* Callstack = nullptr;
#if WITH_EDITOR
	Callstack = &Query.Context.Callstacks_EditorOnly.Emplace_GetRef();
	Callstack->Node = OuterNode;
#endif

	const FValue Value = Get(FVoxelGraphQuery(Query, Callstack));
	Query.Context.Execute();
	return Value.GetValue();
}