// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

struct FVoxelNode;
struct FVoxelGraphNodeRef;

struct VOXELGRAPH_API IVoxelNodeInterface
{
public:
	IVoxelNodeInterface() = default;
	virtual ~IVoxelNodeInterface() = default;

	virtual const FVoxelGraphNodeRef& GetNodeRef() const = 0;

public:
	static void RaiseBufferErrorStatic(const FVoxelGraphNodeRef& Node);

	void RaiseBufferError() const
	{
		RaiseBufferErrorStatic(GetNodeRef());
	}
};

template<typename T>
requires std::derived_from<T, IVoxelNodeInterface>
struct TVoxelMessageTokenFactory<T>
{
	static TSharedRef<FVoxelMessageToken> CreateToken(const T& Node)
	{
		return Node.GetNodeRef().CreateMessageToken();
	}
};