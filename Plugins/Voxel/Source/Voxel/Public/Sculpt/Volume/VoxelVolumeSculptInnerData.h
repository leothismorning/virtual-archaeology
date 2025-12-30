// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMetadataRef.h"
#include "VoxelVolumeBlendMode.h"

struct FVoxelSurfaceTypeBlend;
struct FVoxelDoubleVectorBuffer;
struct FVoxelVolumeTransform;
struct FVoxelVolumeBulkQuery;
struct FVoxelVolumeSparseQuery;
struct FVoxelVolumeDistanceChunk;
struct FVoxelVolumeFastDistanceChunk;
struct FVoxelVolumeMetadataChunk;
struct FVoxelVolumeSurfaceTypeChunk;
struct FVoxelVolumeSculptStampRuntime;

template<typename>
class TVoxelVolumeChunkTree;
template<typename>
class TVoxelVolumeChunkTreeIterator;

class VOXEL_API FVoxelVolumeSculptInnerData
{
public:
	const bool bUseFastDistances;
	const TSharedPtr<TVoxelVolumeChunkTree<FVoxelVolumeDistanceChunk>> DistanceChunkTree_HQ;
	const TSharedPtr<TVoxelVolumeChunkTree<FVoxelVolumeFastDistanceChunk>> DistanceChunkTree_LQ;
	const TSharedRef<TVoxelVolumeChunkTree<FVoxelVolumeSurfaceTypeChunk>> SurfaceTypeChunkTree;
	TVoxelMap<FVoxelMetadataRef, TSharedPtr<TVoxelVolumeChunkTree<FVoxelVolumeMetadataChunk>>> MetadataRefToChunkTree;

	explicit FVoxelVolumeSculptInnerData(bool bUseFastDistances);

	void Serialize(FArchive& Ar);
	void CopyFrom(const FVoxelVolumeSculptInnerData& Other);

public:
	void ApplyShape(
		const FVoxelVolumeBulkQuery& Query,
		const FVoxelVolumeTransform& StampToQuery,
		float Scale,
		EVoxelVolumeBlendMode BlendMode,
		bool bApplyOnVoid) const;

	void ApplyShape(
		const FVoxelVolumeSparseQuery& Query,
		const FVoxelVolumeTransform& StampToQuery,
		float Scale,
		EVoxelVolumeBlendMode BlendMode,
		bool bApplyOnVoid) const;

public:
	void ApplyShape_HQ(
		const FVoxelVolumeBulkQuery& Query,
		const FVoxelVolumeTransform& StampToQuery,
		float Scale,
		EVoxelVolumeBlendMode BlendMode,
		bool bApplyOnVoid) const;

	void ApplyShape_HQ(
		const FVoxelVolumeSparseQuery& Query,
		const FVoxelVolumeTransform& StampToQuery,
		float Scale,
		EVoxelVolumeBlendMode BlendMode,
		bool bApplyOnVoid) const;

public:
	void ApplyShape_LQ(
		const FVoxelVolumeBulkQuery& Query,
		const FVoxelVolumeTransform& StampToQuery,
		float Scale,
		EVoxelVolumeBlendMode BlendMode,
		bool bApplyOnVoid) const;

	void ApplyShape_LQ(
		const FVoxelVolumeSparseQuery& Query,
		const FVoxelVolumeTransform& StampToQuery,
		float Scale,
		EVoxelVolumeBlendMode BlendMode,
		bool bApplyOnVoid) const;

public:
	void ApplySurfaceType(
		const FVoxelVolumeSparseQuery& Query,
		const FVoxelVolumeTransform& StampToQuery,
		float Scale) const;

	void ApplyMetadata(
		const FVoxelMetadataRef& MetadataRef,
		const FVoxelVolumeSparseQuery& Query,
		const FVoxelVolumeTransform& StampToQuery,
		float Scale) const;

public:
	void GetSurfaceTypes(
		const FVoxelDoubleVectorBuffer& Positions,
		TVoxelArray<float>& OutAlphas,
		TVoxelArray<FVoxelSurfaceTypeBlend>& OutSurfaceTypes) const;

	void GetMetadatas(
		const FVoxelMetadataRef& MetadataRef,
		const FVoxelDoubleVectorBuffer& Positions,
		TVoxelArray<float>& OutAlphas,
		TSharedPtr<FVoxelBuffer>& OutMetadatas) const;

private:
	template<typename LambdaType, typename SkipLambdaType>
	static void ProcessDistances_HQ(
		const TVoxelVolumeChunkTreeIterator<FVoxelVolumeDistanceChunk>& Iterator,
		LambdaType Lambda,
		SkipLambdaType SkipLambda);

	template<typename LambdaType, typename SkipLambdaType>
	static void ProcessDistances_LQ(
		const TVoxelVolumeChunkTreeIterator<FVoxelVolumeFastDistanceChunk>& Iterator,
		LambdaType Lambda,
		SkipLambdaType SkipLambda);

	template<typename LambdaType>
	static void ProcessSurfaceTypes(
		const TVoxelVolumeChunkTreeIterator<FVoxelVolumeSurfaceTypeChunk>& Iterator,
		LambdaType Lambda);

	template<typename InnerType, typename LambdaType>
	static void ProcessMetadatas(
		const TVoxelVolumeChunkTreeIterator<FVoxelVolumeMetadataChunk>& Iterator,
		LambdaType Lambda);
};