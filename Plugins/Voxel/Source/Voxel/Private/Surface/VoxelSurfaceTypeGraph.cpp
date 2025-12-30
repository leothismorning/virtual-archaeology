// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Surface/VoxelSurfaceTypeGraph.h"
#include "Surface/VoxelOutputNode_OutputSurface.h"
#include "Surface/VoxelSmartSurfaceType.h"

#if WITH_EDITOR
UVoxelGraph::FFactoryInfo UVoxelSurfaceTypeGraph::GetFactoryInfo()
{
	FFactoryInfo Result;

	Result.DisplayName = GetClass()->GetDisplayNameText().ToString();
	Result.DisplayName.RemoveFromStart("Voxel ");

	Result.Category = "Surface Type";
	Result.Description = "Used to decide which surface to render per vertex";

	Result.Template = LoadObject<UVoxelSurfaceTypeGraph>(nullptr, TEXT("/Voxel/SmartSurfaceGraphTemplate.SmartSurfaceGraphTemplate"));
	return Result;
}
#endif

UScriptStruct* UVoxelSurfaceTypeGraph::GetOutputNodeStruct() const
{
	return FVoxelOutputNode_OutputSurface::StaticStruct();
}

UVoxelSmartSurfaceType* UVoxelSurfaceTypeGraph::GetPreviewSurface()
{
	if (!PreviewSurfaceType)
	{
		PreviewSurfaceType = NewObject<UVoxelSmartSurfaceType>();
		PreviewSurfaceType->Graph = this;
	}

	return PreviewSurfaceType;
}