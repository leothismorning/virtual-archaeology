// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelNodeEvaluator.h"
#include "VoxelGraph.h"
#include "VoxelGraphContext.h"
#include "VoxelGraphEnvironment.h"
#include "VoxelTerminalGraph.h"
#include "VoxelCompiledGraph.h"
#include "VoxelCompiledTerminalGraph.h"
#include "Nodes/VoxelOutputNode.h"

bool FVoxelNodeEvaluator::operator==(const FVoxelNodeEvaluator& Other) const
{
	VOXEL_FUNCTION_COUNTER();

	if (Node != Other.Node ||
		TerminalGraph != Other.TerminalGraph)
	{
		return false;
	}

	return *Environment == *Other.Environment;
}

FVoxelGraphContext FVoxelNodeEvaluator::MakeContext(FVoxelDependencyCollector& DependencyCollector) const
{
	check(IsValid());

	return FVoxelGraphContext(
		*Environment,
		*TerminalGraph,
		DependencyCollector);
}

FVoxelNodeEvaluator FVoxelNodeEvaluator::Create(
	UScriptStruct* Struct,
	const TSharedRef<const FVoxelGraphEnvironment>& Environment,
	const UVoxelTerminalGraph* TerminalGraph,
	const FVoxelNode* Node)
{
	VOXEL_FUNCTION_COUNTER()

	const UVoxelGraph* Graph = Environment->RootCompiledGraph->Graph.Resolve();
	if (!ensure(Graph))
	{
		return {};
	}

	if (Graph->HasMainTerminalGraph() ||
		!Graph->GetBaseGraph_Unsafe())
	{
		if (!TerminalGraph)
		{
			TerminalGraph = &Graph->GetMainTerminalGraph();
		}

		if (!ensure(TerminalGraph->GetOuterUVoxelGraph() == Graph))
		{
			return {};
		}
	}
	else
	{
		for (const UVoxelGraph* BaseGraph : Graph->GetBaseGraphs())
		{
			if (BaseGraph->HasMainTerminalGraph())
			{
				TerminalGraph = &BaseGraph->GetMainTerminalGraph();
			}
		}
	}

	if (!ensure(TerminalGraph))
	{
		return {};
	}

	const FVoxelCompiledTerminalGraph* CompiledTerminalGraph = Environment->RootCompiledGraph->FindTerminalGraph(TerminalGraph->GetGuid());
	if (!CompiledTerminalGraph)
	{
		VOXEL_MESSAGE(Error, "Failed to compile {0}", TerminalGraph);
		return {};
	}

	if (!Node)
	{
		if (!ensure(Struct->IsChildOf(StaticStructFast<FVoxelOutputNode>())))
		{
			return {};
		}

		Node = CompiledTerminalGraph->FindOutputNode(Struct);

		if (!ensureVoxelSlow(Node))
		{
#if WITH_EDITOR
			VOXEL_MESSAGE(Error, "{0} is missing an output node of type {1}",
				TerminalGraph,
				Struct->GetDisplayNameText());
#else
		VOXEL_MESSAGE(Error, "{0} is missing an output node of type {1}",
			TerminalGraph,
			Struct->GetName());
#endif

			return {};
		}
	}

	if (!ensure(Node->IsA(Struct)))
	{
		return {};
	}

	FVoxelNodeEvaluator Result;
	Result.Node = Node;
	Result.Environment = Environment;
	Result.TerminalGraph = CompiledTerminalGraph;
	return Result;
}