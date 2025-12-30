// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Spline/VoxelVolumeSplineGraph.h"
#include "Spline/VoxelOutputNode_OutputVolumeSpline.h"

#if WITH_EDITOR
UVoxelGraph::FFactoryInfo UVoxelVolumeSplineGraph::GetFactoryInfo()
{
	FFactoryInfo Result;

	Result.DisplayName = GetClass()->GetDisplayNameText().ToString();
	Result.DisplayName.RemoveFromStart("Voxel ");

	Result.Category = "Volume";
	Result.Description = "Used by Stamp actors, to create a shapes following a spline";

	Result.Template = LoadObject<UVoxelVolumeSplineGraph>(nullptr, TEXT("/Voxel/VolumeSplineGraphTemplate.VolumeSplineGraphTemplate"));
	return Result;
}
#endif

UScriptStruct* UVoxelVolumeSplineGraph::GetOutputNodeStruct() const
{
	return FVoxelOutputNode_OutputVolumeSpline::StaticStruct();
}