// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Surface/VoxelSmartSurfaceFunctionLibrary.h"
#include "Surface/VoxelSmartSurfaceTypeUtilities.h"
#include "VoxelBufferSplitter.h"
#include "VoxelQuery.h"
#include "Graphs/VoxelStampGraphParameters.h"

void FVoxelGraphParameters::FSmartSurface::Split(
	const FVoxelBufferSplitter& Splitter,
	const TConstVoxelArrayView<FSmartSurface*> OutResult) const
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelInlineArray<FVoxelVectorBuffer*, 8> Buffers;
	FVoxelUtilities::SetNumZeroed(Buffers, Splitter.NumOutputs());

	for (const int32 Index : Splitter.GetValidOutputs())
	{
		Buffers[Index] = &OutResult[Index]->Normals;
	}

	Normals.Split(Splitter, Buffers);
}

FVoxelVectorBuffer UVoxelSmartSurfaceFunctionLibrary::GetVertexNormal() const
{
	if (Query.IsPreview())
	{
		return FVector3f::UpVector;
	}

	const FVoxelGraphParameters::FSmartSurface* Parameter = Query->FindParameter<FVoxelGraphParameters::FSmartSurface>();
	if (!Parameter)
	{
		VOXEL_MESSAGE(Error, "{0}: No vertex data", this);
		return FVector3f::UpVector;
	}

	return Parameter->Normals;
}

FVoxelSurfaceTypeBlendBuffer UVoxelSmartSurfaceFunctionLibrary::ResolveSmartSurfaces(
	const FVoxelSurfaceTypeBlendBuffer& SurfaceTypes,
	const FVoxelDoubleVectorBuffer& Position,
	const FVoxelVectorBuffer& Normal,
	const int32 LOD,
	const FVoxelWeakStackLayer& Layer) const
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelGraphParameters::FQuery* Parameter = Query->FindParameter<FVoxelGraphParameters::FQuery>();
	if (!Parameter)
	{
		VOXEL_MESSAGE(Error, "{0}: Cannot resolve smart surfaces, no query", this);
		return SurfaceTypes;
	}

	FVoxelSurfaceTypeBlendBuffer Result = SurfaceTypes.MakeDeepCopy();

	FVoxelSmartSurfaceTypeUtilities::Resolve(
		LOD,
		Layer,
		Query->Context.DependencyCollector,
		Parameter->Query.Layers,
		Parameter->Query.SurfaceTypeTable,
		Position,
		Normal,
		Result.View());

	return Result;
}