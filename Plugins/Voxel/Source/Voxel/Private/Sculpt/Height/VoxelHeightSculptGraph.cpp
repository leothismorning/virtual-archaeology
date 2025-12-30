// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Height/VoxelHeightSculptGraph.h"
#include "Sculpt/VoxelOutputNode_OutputSculptHeight.h"

#if WITH_EDITOR
UVoxelGraph::FFactoryInfo UVoxelHeightSculptGraph::GetFactoryInfo()
{
	FFactoryInfo Result;

	Result.DisplayName = GetClass()->GetDisplayNameText().ToString();
	Result.DisplayName.RemoveFromStart("Voxel ");

	Result.Category = "Height";
	Result.Description =
		"A graph used in the sculpt system to produce custom shapes for sculpting. "
		"This can only use XY positions and output height.";

	Result.Template = LoadObject<UVoxelHeightSculptGraph>(nullptr, TEXT("/Voxel/HeightSculptGraphTemplate.HeightSculptGraphTemplate"));
	return Result;
}
#endif

UScriptStruct* UVoxelHeightSculptGraph::GetOutputNodeStruct() const
{
	return FVoxelOutputNode_OutputSculptHeight::StaticStruct();
}