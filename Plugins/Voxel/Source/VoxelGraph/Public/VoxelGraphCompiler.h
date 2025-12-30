// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelCompilationGraph.h"
#include "VoxelTerminalGraphRuntime.h"

class VOXELGRAPH_API FVoxelGraphCompiler : public Voxel::Graph::FGraph
{
public:
	using FPin = Voxel::Graph::FPin;
	using FNode = Voxel::Graph::FNode;
	using ENodeType = Voxel::Graph::ENodeType;
	using EPinDirection = Voxel::Graph::EPinDirection;

public:
	const UVoxelTerminalGraph& TerminalGraph;
	const FVoxelSerializedGraph& SerializedGraph;

	explicit FVoxelGraphCompiler(const UVoxelTerminalGraph& TerminalGraph);

	bool LoadSerializedGraph(const FOnVoxelGraphChanged& OnTranslated);

public:
	void AddPreviewNode();
	void AddRangeNodes();
	void AddPreviewValueNodes();
	void RemoveSplitPins();
	void FixPositionPins();
	void FixSplineKeyPins();
	void AddWildcardErrors();
	void AddNoDefaultErrors();
	void CheckParameters() const;
	void CheckFunctionInputs() const;
	void CheckFunctionOutputs() const;
	void AddToBuffer();
	void RemoveLocalVariables();
	void CollapseInputs();
	void ReplaceTemplates();
	void RemovePassthroughs();
	void RemoveNodesNotLinkedToQueryableNodes();
	void CheckForLoops();

private:
	bool ReplaceTemplatesImpl();
	void InitializeTemplatesPassthroughNodes(FNode& Node);
	void RemoveNodesImpl(const TVoxelArray<const FNode*>& RootNodes);
};