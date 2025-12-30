// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelHeightSculptStampRef.h"
#include "VoxelStampBlueprintFunctionLibrary.h"
#include "VoxelHeightSculptStamp_K2.generated.h"

////////////////////////////////////////////////////
///////// The code below is auto-generated /////////
////////////////////////////////////////////////////

UCLASS()
class VOXEL_API UVoxelHeightSculptStamp_K2 : public UVoxelStampBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Casting", meta = (ExpandEnumAsExecs = "Result"))
	static FVoxelHeightSculptStampRef CastToHeightSculptStamp(const FVoxelStampRef Stamp, EVoxelStampCastResult& Result)
	{
		return CastToStampImpl<FVoxelHeightSculptStamp>(Stamp, Result);
	}

	// Make a copy of this stamp
	// You can then call Set XXXX on the copy without having the original stamp be modified
	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Height Sculpt", DisplayName = "Get Height Sculpt Stamp")
	static void MakeCopy(const FVoxelHeightSculptStampRef Stamp, FVoxelHeightSculptStampRef& Copy)
	{
		Copy = FVoxelHeightSculptStampRef(Stamp.MakeCopy());
	}

	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Height Sculpt", DisplayName = "Make Voxel Height Sculpt Stamp", meta = (Keywords = "Construct, Create", ScaleXY = "100.000000", bRelativeHeight = "false", Layer = "/Voxel/Default/DefaultHeightLayer.DefaultHeightLayer", BlendMode = "Override", Transform = "0.000000,0.000000,0.000000|0.000000,0.000000,-0.000000|1.000000,1.000000,1.000000", Behavior = "AffectAll", Priority = "0", Smoothness = "100.000000", MetadataOverrides = "(Overrides=)", LODRange = "(Min=0,Max=32)", bDisableStampSelection = "false", BoundsExtension = "0.100000", AutoCreateRefTerm = "AdditionalLayers"))
	static void Make(
		FVoxelHeightSculptStampRef& Stamp,
		UPARAM(meta = (DisplayName = "Scale XY", ToolTip = "")) float ScaleXY,
		UPARAM(meta = (DisplayName = "Relative Height", ToolTip = "If true height will be stored relative to the previous stamp heights")) bool bRelativeHeight,
		UPARAM(meta = (DisplayName = "Stack Override", ToolTip = "Use this if this stamp is not rendered in the Voxel World stack\nThis stack will be used during sculpting to query the distances before any sculpt is applied")) UVoxelLayerStack* StackOverride,
		UPARAM(meta = (DisplayName = "Layer", ToolTip = "Layer that this stamps belong to\nYou can control the order of layers in Layer Stacks\nYou can select the layer stack to use in your Voxel World or PCG Sampler settings")) UVoxelHeightLayer* Layer,
		UPARAM(meta = (DisplayName = "Blend Mode", ToolTip = "")) EVoxelHeightBlendMode BlendMode,
		UPARAM(meta = (DisplayName = "Additional Layers", ToolTip = "")) TArray<UVoxelHeightLayer*> AdditionalLayers,
		UPARAM(meta = (DisplayName = "Transform", ToolTip = "")) FTransform Transform,
		UPARAM(meta = (DisplayName = "Behavior", ToolTip = "")) EVoxelStampBehavior Behavior,
		UPARAM(meta = (DisplayName = "Priority", ToolTip = "Priority of the stamp within its layer\nHigher priority stamps will be applied last")) int32 Priority,
		UPARAM(meta = (DisplayName = "Smoothness", ToolTip = "")) float Smoothness,
		UPARAM(meta = (DisplayName = "Metadata Overrides", ToolTip = "")) FVoxelMetadataOverrides MetadataOverrides,
		UPARAM(meta = (DisplayName = "LOD Range", ToolTip = "This stamp will only be applied on LODs within this range (inclusive)")) FInt32Interval LODRange,
		UPARAM(meta = (DisplayName = "Disable Stamp Selection", ToolTip = "If true you won\'t be able to select this stamp by clicking on it")) bool bDisableStampSelection,
		UPARAM(meta = (DisplayName = "Bounds Extension", ToolTip = "By how much to extend the bounds, relative to the bounds size\nIncrease this if you are using a high smoothness\nIncreasing this will lead to more stamps being sampled per voxel, increasing generation cost")) float BoundsExtension)
	{
		Stamp = FVoxelHeightSculptStampRef::New();
		Stamp->ScaleXY = ScaleXY;
		Stamp->bRelativeHeight = bRelativeHeight;
		Stamp->StackOverride = StackOverride;
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
		Stamp->BoundsExtension = BoundsExtension;
	}

	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Height Sculpt", DisplayName = "Break Voxel Height Sculpt Stamp", meta = (Keywords = "Break"))
	static void Break(
		const FVoxelHeightSculptStampRef Stamp,
		UPARAM(meta = (DisplayName = "Scale XY", ToolTip = "")) float& ScaleXY,
		UPARAM(meta = (DisplayName = "Relative Height", ToolTip = "If true height will be stored relative to the previous stamp heights")) bool& bRelativeHeight,
		UPARAM(meta = (DisplayName = "Stack Override", ToolTip = "Use this if this stamp is not rendered in the Voxel World stack\nThis stack will be used during sculpting to query the distances before any sculpt is applied")) UVoxelLayerStack*& StackOverride,
		UPARAM(meta = (DisplayName = "Layer", ToolTip = "Layer that this stamps belong to\nYou can control the order of layers in Layer Stacks\nYou can select the layer stack to use in your Voxel World or PCG Sampler settings")) UVoxelHeightLayer*& Layer,
		UPARAM(meta = (DisplayName = "Blend Mode", ToolTip = "")) EVoxelHeightBlendMode& BlendMode,
		UPARAM(meta = (DisplayName = "Additional Layers", ToolTip = "")) TArray<UVoxelHeightLayer*>& AdditionalLayers,
		UPARAM(meta = (DisplayName = "Transform", ToolTip = "")) FTransform& Transform,
		UPARAM(meta = (DisplayName = "Behavior", ToolTip = "")) EVoxelStampBehavior& Behavior,
		UPARAM(meta = (DisplayName = "Priority", ToolTip = "Priority of the stamp within its layer\nHigher priority stamps will be applied last")) int32& Priority,
		UPARAM(meta = (DisplayName = "Smoothness", ToolTip = "")) float& Smoothness,
		UPARAM(meta = (DisplayName = "Metadata Overrides", ToolTip = "")) FVoxelMetadataOverrides& MetadataOverrides,
		UPARAM(meta = (DisplayName = "LOD Range", ToolTip = "This stamp will only be applied on LODs within this range (inclusive)")) FInt32Interval& LODRange,
		UPARAM(meta = (DisplayName = "Disable Stamp Selection", ToolTip = "If true you won\'t be able to select this stamp by clicking on it")) bool& bDisableStampSelection,
		UPARAM(meta = (DisplayName = "Bounds Extension", ToolTip = "By how much to extend the bounds, relative to the bounds size\nIncrease this if you are using a high smoothness\nIncreasing this will lead to more stamps being sampled per voxel, increasing generation cost")) float& BoundsExtension)
	{
		ScaleXY = FVoxelUtilities::MakeSafe<float>();
		bRelativeHeight = FVoxelUtilities::MakeSafe<bool>();
		StackOverride = FVoxelUtilities::MakeSafe<UVoxelLayerStack*>();
		Layer = FVoxelUtilities::MakeSafe<UVoxelHeightLayer*>();
		BlendMode = FVoxelUtilities::MakeSafe<EVoxelHeightBlendMode>();
		AdditionalLayers = FVoxelUtilities::MakeSafe<TArray<UVoxelHeightLayer*>>();
		Transform = FVoxelUtilities::MakeSafe<FTransform>();
		Behavior = FVoxelUtilities::MakeSafe<EVoxelStampBehavior>();
		Priority = FVoxelUtilities::MakeSafe<int32>();
		Smoothness = FVoxelUtilities::MakeSafe<float>();
		MetadataOverrides = FVoxelUtilities::MakeSafe<FVoxelMetadataOverrides>();
		LODRange = FVoxelUtilities::MakeSafe<FInt32Interval>();
		bDisableStampSelection = FVoxelUtilities::MakeSafe<bool>();
		BoundsExtension = FVoxelUtilities::MakeSafe<float>();

		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		ScaleXY = Stamp->ScaleXY;
		bRelativeHeight = Stamp->bRelativeHeight;
		StackOverride = Stamp->StackOverride;
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
		BoundsExtension = Stamp->BoundsExtension;
	}

	// Get Scale XY
	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Height Sculpt", DisplayName = "Get Scale XY")
	static void GetScaleXY(UPARAM(Required) FVoxelHeightSculptStampRef Stamp, float& ScaleXY)
	{
		ScaleXY = FVoxelUtilities::MakeSafe<float>();

		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		ScaleXY = Stamp->ScaleXY;
	}

	// Set Scale XY
	// This will automatically refresh the stamp
	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Height Sculpt", DisplayName = "Set Scale XY")
	static void SetScaleXY(UPARAM(Required) FVoxelHeightSculptStampRef Stamp, UPARAM(meta = (DisplayName = "Stamp")) FVoxelHeightSculptStampRef& OutStamp, float ScaleXY)
	{
		OutStamp = Stamp;
		
		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Stamp->ScaleXY = ScaleXY;
		Stamp.Update();
	}

	// Get Relative Height
	//
	// If true height will be stored relative to the previous stamp heights
	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Height Sculpt", DisplayName = "Get Relative Height")
	static void GetRelativeHeight(UPARAM(Required) FVoxelHeightSculptStampRef Stamp, bool& bRelativeHeight)
	{
		bRelativeHeight = FVoxelUtilities::MakeSafe<bool>();

		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		bRelativeHeight = Stamp->bRelativeHeight;
	}

	// Set Relative Height
	// This will automatically refresh the stamp
	//
	// If true height will be stored relative to the previous stamp heights
	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Height Sculpt", DisplayName = "Set Relative Height")
	static void SetRelativeHeight(UPARAM(Required) FVoxelHeightSculptStampRef Stamp, UPARAM(meta = (DisplayName = "Stamp")) FVoxelHeightSculptStampRef& OutStamp, bool bRelativeHeight)
	{
		OutStamp = Stamp;
		
		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Stamp->bRelativeHeight = bRelativeHeight;
		Stamp.Update();
	}

	// Get Stack Override
	//
	// Use this if this stamp is not rendered in the Voxel World stack
	// This stack will be used during sculpting to query the distances before any sculpt is applied
	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Height Sculpt", DisplayName = "Get Stack Override")
	static void GetStackOverride(UPARAM(Required) FVoxelHeightSculptStampRef Stamp, UVoxelLayerStack*& StackOverride)
	{
		StackOverride = FVoxelUtilities::MakeSafe<UVoxelLayerStack*>();

		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		StackOverride = Stamp->StackOverride;
	}

	// Set Stack Override
	// This will automatically refresh the stamp
	//
	// Use this if this stamp is not rendered in the Voxel World stack
	// This stack will be used during sculpting to query the distances before any sculpt is applied
	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Height Sculpt", DisplayName = "Set Stack Override")
	static void SetStackOverride(UPARAM(Required) FVoxelHeightSculptStampRef Stamp, UPARAM(meta = (DisplayName = "Stamp")) FVoxelHeightSculptStampRef& OutStamp, UVoxelLayerStack* StackOverride)
	{
		OutStamp = Stamp;
		
		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Stamp->StackOverride = StackOverride;
		Stamp.Update();
	}
};