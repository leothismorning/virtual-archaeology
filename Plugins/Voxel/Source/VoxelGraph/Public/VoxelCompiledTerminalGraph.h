// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

struct FVoxelNode;
struct FVoxelOutputNode;
struct FVoxelNode_FunctionOutput;
struct FVoxelNode_CustomizeParameter;

namespace Voxel::Graph
{
	class FGraph;
}

// Will keep pin default value objects alive
class VOXELGRAPH_API FVoxelCompiledTerminalGraph
{
public:
	const FGuid TerminalGraphGuid;
	const int32 NumNodes;
	const int32 NumPins;

	FORCEINLINE const TSharedRef<const Voxel::Graph::FGraph>& GetGraph() const
	{
		return Graph;
	}
	FORCEINLINE bool OwnsNode(const FVoxelNode* Node) const
	{
		return OwnedNodes.Contains(Node);
	}
	FORCEINLINE const FVoxelOutputNode* FindOutputNode(UScriptStruct* Struct) const
	{
		return StructToOutputNode.FindRef(Struct);
	}
	FORCEINLINE const FVoxelOutputNode* GetMainOutputNode() const
	{
		return MainOutputNode;
	}
	FORCEINLINE TConstVoxelArrayView<const FVoxelNode_FunctionOutput*> GetFunctionOutputs() const
	{
		return FunctionOutputs;
	}
	FORCEINLINE const FVoxelNode_CustomizeParameter* FindCustomizeParameterNode(const FGuid ParameterGuid) const
	{
		return GuidToCustomizeParameter.FindRef(ParameterGuid);
	}

	UE_NONCOPYABLE(FVoxelCompiledTerminalGraph);

private:
	const TSharedRef<const Voxel::Graph::FGraph> Graph;
	const TVoxelArray<TSharedPtr<FVoxelNode>> NodeRefs;
	const TVoxelSet<const FVoxelNode*> OwnedNodes;
	const TVoxelMap<UScriptStruct*, const FVoxelOutputNode*> StructToOutputNode;
	const FVoxelOutputNode* MainOutputNode;
	const TVoxelArray<const FVoxelNode_FunctionOutput*> FunctionOutputs;
	const TVoxelMap<FGuid, const FVoxelNode_CustomizeParameter*> GuidToCustomizeParameter;
	TSharedPtr<FVoxelDependencyTracker> DependencyTracker;

	FVoxelCompiledTerminalGraph(
		const FGuid TerminalGraphGuid,
		const int32 NumNodes,
		const int32 NumPins,
		const TSharedRef<const Voxel::Graph::FGraph>& Graph,
		TVoxelArray<TSharedPtr<FVoxelNode>>&& NodeRefs,
		TVoxelSet<const FVoxelNode*>&& OwnedNodes,
		TVoxelMap<UScriptStruct*, const FVoxelOutputNode*>&& StructToOutputNode,
		const FVoxelOutputNode* MainOutputNode,
		TVoxelArray<const FVoxelNode_FunctionOutput*>&& FunctionOutputs,
		TVoxelMap<FGuid, const FVoxelNode_CustomizeParameter*>&& GuidToCustomizeParameter)
		: TerminalGraphGuid(TerminalGraphGuid)
		, NumNodes(NumNodes)
		, NumPins(NumPins)
		, Graph(Graph)
		, NodeRefs(MoveTemp(NodeRefs))
		, OwnedNodes(MoveTemp(OwnedNodes))
		, StructToOutputNode(MoveTemp(StructToOutputNode))
		, MainOutputNode(MainOutputNode)
		, FunctionOutputs(MoveTemp(FunctionOutputs))
		, GuidToCustomizeParameter(MoveTemp(GuidToCustomizeParameter))
	{
	}

	friend class UVoxelTerminalGraphRuntime;
};