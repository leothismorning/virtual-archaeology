// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Volume/VoxelVolumeSculptGraph.h"
#include "Sculpt/VoxelOutputNode_OutputSculptDistance.h"

#if WITH_EDITOR
UVoxelGraph::FFactoryInfo UVoxelVolumeSculptGraph::GetFactoryInfo()
{
	FFactoryInfo Result;

	Result.DisplayName = GetClass()->GetDisplayNameText().ToString();
	Result.DisplayName.RemoveFromStart("Voxel ");

	Result.Category = "Volume";
	Result.Description =
		"A graph used in the sculpt system to produce custom shapes for sculpting. "
		"This uses XYZ positions and outputs distance (negative meaning inside of shape, positive - outside).";

	Result.Template = LoadObject<UVoxelVolumeSculptGraph>(nullptr, TEXT("/Voxel/VolumeSculptGraphTemplate.VolumeSculptGraphTemplate"));
	return Result;
}
#endif

UScriptStruct* UVoxelVolumeSculptGraph::GetOutputNodeStruct() const
{
	return FVoxelOutputNode_OutputSculptDistance::StaticStruct();
}