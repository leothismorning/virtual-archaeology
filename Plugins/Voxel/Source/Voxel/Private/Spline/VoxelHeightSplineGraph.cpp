// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Spline/VoxelHeightSplineGraph.h"
#include "Spline/VoxelOutputNode_OutputHeightSpline.h"

#if WITH_EDITOR
UVoxelGraph::FFactoryInfo UVoxelHeightSplineGraph::GetFactoryInfo()
{
	FFactoryInfo Result;

	Result.DisplayName = GetClass()->GetDisplayNameText().ToString();
	Result.DisplayName.RemoveFromStart("Voxel ");

	Result.Category = "Height";
	Result.Description =
		"Used by Stamp actors, to create a shapes following a spline. "
		"This is using XY positions and outputting height, so it cannot produce overhangs, caves. "
		"They are usually used for roads or rivers.";

	Result.Template = LoadObject<UVoxelHeightSplineGraph>(nullptr, TEXT("/Voxel/HeightSplineGraphTemplate.HeightSplineGraphTemplate"));
	return Result;
}
#endif

UScriptStruct* UVoxelHeightSplineGraph::GetOutputNodeStruct() const
{
	return FVoxelOutputNode_OutputHeightSpline::StaticStruct();
}