// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelPlaneShape.h"
#include "VoxelShapeStampRef.h"
#include "VoxelStampBlueprintFunctionLibrary.h"
#include "VoxelPlaneShape_K2.generated.h"

////////////////////////////////////////////////////
///////// The code below is auto-generated /////////
////////////////////////////////////////////////////

UCLASS()
class VOXEL_API UVoxelPlaneShape_K2 : public UVoxelStampBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Shape|Plane", DisplayName = "Make Voxel Plane Shape Stamp", meta = (Keywords = "Construct, Create", Layer = "/Voxel/Default/DefaultVolumeLayer.DefaultVolumeLayer", BlendMode = "Additive", Transform = "0.000000,0.000000,0.000000|0.000000,0.000000,-0.000000|1.000000,1.000000,1.000000", Behavior = "AffectAll", Priority = "0", Smoothness = "100.000000", MetadataOverrides = "(Overrides=)", LODRange = "(Min=0,Max=32)", bDisableStampSelection = "false", bApplyOnVoid = "true", BoundsExtension = "1.000000", Size = "(X=1000.000000,Y=1000.000000)", Height = "1.000000", AutoCreateRefTerm = "AdditionalLayers"))
	static void Make(
		FVoxelShapeStampRef& Stamp,
		UPARAM(meta = (DisplayName = "Size", ToolTip = "")) FVector2D Size,
		UPARAM(meta = (DisplayName = "Height", ToolTip = "Relative to Size, by how much to extend the distance field up and down")) double Height,
		UPARAM(meta = (DisplayName = "Surface Type", ToolTip = "")) UVoxelSurfaceTypeInterface* SurfaceType,
		UPARAM(meta = (DisplayName = "Layer", ToolTip = "Layer that this stamps belong to\nYou can control the order of layers in Layer Stacks\nYou can select the layer stack to use in your Voxel World or PCG Sampler settings")) UVoxelVolumeLayer* Layer,
		UPARAM(meta = (DisplayName = "Blend Mode", ToolTip = "")) EVoxelVolumeBlendMode BlendMode,
		UPARAM(meta = (DisplayName = "Additional Layers", ToolTip = "")) TArray<UVoxelVolumeLayer*> AdditionalLayers,
		UPARAM(meta = (DisplayName = "Transform", ToolTip = "")) FTransform Transform,
		UPARAM(meta = (DisplayName = "Behavior", ToolTip = "")) EVoxelStampBehavior Behavior,
		UPARAM(meta = (DisplayName = "Priority", ToolTip = "Priority of the stamp within its layer\nHigher priority stamps will be applied last")) int32 Priority,
		UPARAM(meta = (DisplayName = "Smoothness", ToolTip = "")) float Smoothness,
		UPARAM(meta = (DisplayName = "Metadata Overrides", ToolTip = "")) FVoxelMetadataOverrides MetadataOverrides,
		UPARAM(meta = (DisplayName = "LOD Range", ToolTip = "This stamp will only be applied on LODs within this range (inclusive)")) FInt32Interval LODRange,
		UPARAM(meta = (DisplayName = "Disable Stamp Selection", ToolTip = "If true you won\'t be able to select this stamp by clicking on it")) bool bDisableStampSelection,
		UPARAM(meta = (DisplayName = "Apply on Void", ToolTip = "If false, this stamp will only apply on parts where another stamp has been applied first\nThis is useful to avoid having stamps going beyond world bounds\nOnly used if BlendMode is not Override nor Intersect")) bool bApplyOnVoid,
		UPARAM(meta = (DisplayName = "Bounds Extension", ToolTip = "By how much to extend the bounds, relative to the bounds size\nIncrease this if you are using a high smoothness\nIncreasing this will lead to more stamps being sampled per voxel, increasing generation cost")) float BoundsExtension)
	{
		Stamp = FVoxelShapeStampRef::New();
		Stamp->SurfaceType = SurfaceType;
		Stamp->Layer = Layer;
		Stamp->BlendMode = BlendMode;
		Stamp->AdditionalLayers = AdditionalLayers;
		Stamp->Transform = Transform;
		Stamp->Behavior = Behavior;
		Stamp->Priority = Priority;
		Stamp->Smoothness = Smoothness;
		Stamp->MetadataOverrides = MetadataOverrides;
		Stamp->LODRange = LODRange;
		Stamp->bDisableStampSelection = bDisableStampSelection;
		Stamp->bApplyOnVoid = bApplyOnVoid;
		Stamp->BoundsExtension = BoundsExtension;
		TVoxelInstancedStruct<FVoxelPlaneShape> Shape(FVoxelPlaneShape::StaticStruct());
		Shape->Size = Size;
		Shape->Height = Height;
		Stamp->Shape = Shape;
	}

	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Shape|Plane", DisplayName = "Break Voxel Plane Shape Stamp", meta = (Keywords = "Break"))
	static void Break(
		const FVoxelShapeStampRef Stamp,
		UPARAM(meta = (DisplayName = "Size", ToolTip = "")) FVector2D& Size,
		UPARAM(meta = (DisplayName = "Height", ToolTip = "Relative to Size, by how much to extend the distance field up and down")) double& Height,
		UPARAM(meta = (DisplayName = "Surface Type", ToolTip = "")) UVoxelSurfaceTypeInterface*& SurfaceType,
		UPARAM(meta = (DisplayName = "Layer", ToolTip = "Layer that this stamps belong to\nYou can control the order of layers in Layer Stacks\nYou can select the layer stack to use in your Voxel World or PCG Sampler settings")) UVoxelVolumeLayer*& Layer,
		UPARAM(meta = (DisplayName = "Blend Mode", ToolTip = "")) EVoxelVolumeBlendMode& BlendMode,
		UPARAM(meta = (DisplayName = "Additional Layers", ToolTip = "")) TArray<UVoxelVolumeLayer*>& AdditionalLayers,
		UPARAM(meta = (DisplayName = "Transform", ToolTip = "")) FTransform& Transform,
		UPARAM(meta = (DisplayName = "Behavior", ToolTip = "")) EVoxelStampBehavior& Behavior,
		UPARAM(meta = (DisplayName = "Priority", ToolTip = "Priority of the stamp within its layer\nHigher priority stamps will be applied last")) int32& Priority,
		UPARAM(meta = (DisplayName = "Smoothness", ToolTip = "")) float& Smoothness,
		UPARAM(meta = (DisplayName = "Metadata Overrides", ToolTip = "")) FVoxelMetadataOverrides& MetadataOverrides,
		UPARAM(meta = (DisplayName = "LOD Range", ToolTip = "This stamp will only be applied on LODs within this range (inclusive)")) FInt32Interval& LODRange,
		UPARAM(meta = (DisplayName = "Disable Stamp Selection", ToolTip = "If true you won\'t be able to select this stamp by clicking on it")) bool& bDisableStampSelection,
		UPARAM(meta = (DisplayName = "Apply on Void", ToolTip = "If false, this stamp will only apply on parts where another stamp has been applied first\nThis is useful to avoid having stamps going beyond world bounds\nOnly used if BlendMode is not Override nor Intersect")) bool& bApplyOnVoid,
		UPARAM(meta = (DisplayName = "Bounds Extension", ToolTip = "By how much to extend the bounds, relative to the bounds size\nIncrease this if you are using a high smoothness\nIncreasing this will lead to more stamps being sampled per voxel, increasing generation cost")) float& BoundsExtension)
	{
		SurfaceType = FVoxelUtilities::MakeSafe<UVoxelSurfaceTypeInterface*>();
		Layer = FVoxelUtilities::MakeSafe<UVoxelVolumeLayer*>();
		BlendMode = FVoxelUtilities::MakeSafe<EVoxelVolumeBlendMode>();
		AdditionalLayers = FVoxelUtilities::MakeSafe<TArray<UVoxelVolumeLayer*>>();
		Transform = FVoxelUtilities::MakeSafe<FTransform>();
		Behavior = FVoxelUtilities::MakeSafe<EVoxelStampBehavior>();
		Priority = FVoxelUtilities::MakeSafe<int32>();
		Smoothness = FVoxelUtilities::MakeSafe<float>();
		MetadataOverrides = FVoxelUtilities::MakeSafe<FVoxelMetadataOverrides>();
		LODRange = FVoxelUtilities::MakeSafe<FInt32Interval>();
		bDisableStampSelection = FVoxelUtilities::MakeSafe<bool>();
		bApplyOnVoid = FVoxelUtilities::MakeSafe<bool>();
		BoundsExtension = FVoxelUtilities::MakeSafe<float>();
		Size = FVoxelUtilities::MakeSafe<FVector2D>();
		Height = FVoxelUtilities::MakeSafe<double>();

		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		FVoxelPlaneShape* Shape = Stamp->Shape->As<FVoxelPlaneShape>();
		if (!Shape)
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		SurfaceType = Stamp->SurfaceType;
		Layer = Stamp->Layer;
		BlendMode = Stamp->BlendMode;
		AdditionalLayers = Stamp->AdditionalLayers;
		Transform = Stamp->Transform;
		Behavior = Stamp->Behavior;
		Priority = Stamp->Priority;
		Smoothness = Stamp->Smoothness;
		MetadataOverrides = Stamp->MetadataOverrides;
		LODRange = Stamp->LODRange;
		bDisableStampSelection = Stamp->bDisableStampSelection;
		bApplyOnVoid = Stamp->bApplyOnVoid;
		BoundsExtension = Stamp->BoundsExtension;
		Size = Shape->Size;
		Height = Shape->Height;
	}

	// Get Size
	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Shape|Plane", DisplayName = "Get Size")
	static void GetSize(UPARAM(Required) FVoxelShapeStampRef Stamp, FVector2D& Size)
	{
		Size = FVoxelUtilities::MakeSafe<FVector2D>();

		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		FVoxelPlaneShape* Shape = Stamp->Shape->As<FVoxelPlaneShape>();
		if (!Shape)
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Size = Shape->Size;
	}

	// Set Size
	// This will automatically refresh the stamp
	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Shape|Plane", DisplayName = "Set Size")
	static void SetSize(UPARAM(Required) FVoxelShapeStampRef Stamp, UPARAM(meta = (DisplayName = "Stamp")) FVoxelShapeStampRef& OutStamp, FVector2D Size)
	{
		OutStamp = Stamp;
		
		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		FVoxelPlaneShape* Shape = Stamp->Shape->As<FVoxelPlaneShape>();
		if (!Shape)
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Shape->Size = Size;
		Stamp.Update();
	}

	// Get Height
	//
	// Relative to Size, by how much to extend the distance field up and down
	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Shape|Plane", DisplayName = "Get Height")
	static void GetHeight(UPARAM(Required) FVoxelShapeStampRef Stamp, double& Height)
	{
		Height = FVoxelUtilities::MakeSafe<double>();

		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		FVoxelPlaneShape* Shape = Stamp->Shape->As<FVoxelPlaneShape>();
		if (!Shape)
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Height = Shape->Height;
	}

	// Set Height
	// This will automatically refresh the stamp
	//
	// Relative to Size, by how much to extend the distance field up and down
	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Shape|Plane", DisplayName = "Set Height")
	static void SetHeight(UPARAM(Required) FVoxelShapeStampRef Stamp, UPARAM(meta = (DisplayName = "Stamp")) FVoxelShapeStampRef& OutStamp, double Height)
	{
		OutStamp = Stamp;
		
		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		FVoxelPlaneShape* Shape = Stamp->Shape->As<FVoxelPlaneShape>();
		if (!Shape)
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Shape->Height = Height;
		Stamp.Update();
	}
};