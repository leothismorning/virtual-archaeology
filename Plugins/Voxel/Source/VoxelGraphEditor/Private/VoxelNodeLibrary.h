// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelNode.h"

struct FVoxelNodeLibrary
{
public:
	FVoxelNodeLibrary();

	static TConstVoxelArrayView<TSharedRef<const FVoxelNode>> GetNodes()
	{
		return Get().Nodes;
	}

	static TSharedPtr<const FVoxelNode> FindMakeNode(const FVoxelPinType& Type)
	{
		return Get().TypeToMakeNode.FindRef(Type);
	}
	static TSharedPtr<const FVoxelNode> FindBreakNode(const FVoxelPinType& Type)
	{
		return Get().TypeToBreakNode.FindRef(Type);
	}

	static TSharedPtr<const FVoxelNode> FindCastNode(const FVoxelPinType& From, const FVoxelPinType& To)
	{
		return Get().FromTypeAndToTypeToCastNode.FindRef({ From, To });
	}

	template<typename T>
	static TSharedPtr<const T> GetNodeInstance()
	{
		return StaticCastSharedPtr<const T>(Get().StructToNode.FindRef(T::StaticStruct()));
	}

	static TSharedPtr<const FVoxelNode> FindNode(const UScriptStruct* Struct)
	{
		return Get().StructToNode.FindRef(Struct);
	}

	static TSharedPtr<const FVoxelNode> FindNode(const UFunction* Function)
	{
		return Get().FunctionToNode.FindRef(Function);
	}

private:
	TVoxelArray<TSharedRef<const FVoxelNode>> Nodes;
	TVoxelMap<FVoxelPinType, TSharedPtr<const FVoxelNode>> TypeToMakeNode;
	TVoxelMap < FVoxelPinType, TSharedPtr<const FVoxelNode>> TypeToBreakNode;
	TVoxelMap<TPair<FVoxelPinType, FVoxelPinType>, TSharedPtr<const FVoxelNode>> FromTypeAndToTypeToCastNode;
	TVoxelMap<const UScriptStruct*, TSharedPtr<const FVoxelNode>> StructToNode;
	TVoxelMap<const UFunction*, TSharedPtr<const FVoxelNode>> FunctionToNode;

	static const FVoxelNodeLibrary& Get();
};