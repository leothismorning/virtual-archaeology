// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Buffer/VoxelFloatBuffers.h"
#include "Buffer/VoxelGraphStaticMeshBuffer.h"

struct FVoxelPointSet;
struct FVoxelGraphNodeRef;
struct FVoxelPointIdBuffer;

struct VOXELPCG_API FVoxelPointUtilities
{
public:
	static TSharedPtr<const FVoxelBuffer> FindExtractedAttribute(
		const FVoxelPointSet& PointSet,
		FName AttributeName,
		FName& OutName,
		TVoxelArray<FString>& OutExtractors);

public:
	static void SetExtractedAttribute(
		FVoxelBuffer& Attribute,
		const TSharedRef<const FVoxelBuffer>& NewValue,
		const TArray<FString>& Extractors,
		int32 ExtractorIndex,
		FName AttributeName,
		const FVoxelGraphNodeRef& Node);

public:
	static TSharedPtr<const FVoxelBuffer> ExtractVectorComponent(
		const FVoxelBuffer& Buffer,
		const FString& Extractor,
		const FVoxelPinType& ReturnPinType,
		FName AttributeName,
		const FVoxelGraphNodeRef& Node);

	static TSharedPtr<const FVoxelBuffer> ExtractVector(
		const TSharedRef<const FVoxelBuffer>& Buffer,
		const FString& Extractor,
		const FVoxelPinType& ReturnPinType,
		FName AttributeName,
		const FVoxelGraphNodeRef& Node);

	static TSharedPtr<const FVoxelBuffer> ExtractQuat(
		const TSharedRef<const FVoxelQuaternionBuffer>& Buffer,
		const FString& Extractor,
		const FVoxelPinType& ReturnPinType,
		FName AttributeName,
		const FVoxelGraphNodeRef& Node);

	static TSharedPtr<const FVoxelBuffer> ExtractTransform(
		const TSharedRef<const FVoxelTransformBuffer>& Buffer,
		const FString& Extractor,
		const FVoxelPinType& ReturnPinType,
		FName AttributeName,
		const FVoxelGraphNodeRef& Node);

public:
	static FVoxelSeedBuffer PointIdToSeed(const FVoxelPointIdBuffer& Buffer, const FVoxelSeed& Seed);
};