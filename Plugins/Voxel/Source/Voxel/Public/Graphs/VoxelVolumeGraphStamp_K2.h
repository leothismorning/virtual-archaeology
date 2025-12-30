// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelVolumeGraphStampRef.h"
#include "VoxelStampBlueprintFunctionLibrary.h"
#include "VoxelVolumeGraphStamp_K2.generated.h"

////////////////////////////////////////////////////
///////// The code below is auto-generated /////////
////////////////////////////////////////////////////

UCLASS()
class VOXEL_API UVoxelVolumeGraphStamp_K2 : public UVoxelStampBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Casting", meta = (ExpandEnumAsExecs = "Result"))
	static FVoxelVolumeGraphStampRef CastToVolumeGraphStamp(const FVoxelStampRef Stamp, EVoxelStampCastResult& Result)
	{
		return CastToStampImpl<FVoxelVolumeGraphStamp>(Stamp, Result);
	}

	// Make a copy of this stamp
	// You can then call Set XXXX on the copy without having the original stamp be modified
	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Volume Graph", DisplayName = "Get Volume Graph Stamp")
	static void MakeCopy(const FVoxelVolumeGraphStampRef Stamp, FVoxelVolumeGraphStampRef& Copy)
	{
		Copy = FVoxelVolumeGraphStampRef(Stamp.MakeCopy());
	}

	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Volume Graph", DisplayName = "Make Voxel Volume Graph Stamp", meta = (Keywords = "Construct, Create", Layer = "/Voxel/Default/DefaultVolumeLayer.DefaultVolumeLayer", BlendMode = "Additive", Transform = "0.000000,0.000000,0.000000|0.000000,0.000000,-0.000000|1.000000,1.000000,1.000000", Behavior = "AffectAll", Priority = "0", Smoothness = "100.000000", MetadataOverrides = "(Overrides=)", LODRange = "(Min=0,Max=32)", bDisableStampSelection = "false", bApplyOnVoid = "true", BoundsExtension = "1.000000", AutoCreateRefTerm = "AdditionalLayers"))
	static void Make(
		FVoxelVolumeGraphStampRef& Stamp,
		UPARAM(meta = (DisplayName = "Graph", ToolTip = "")) UVoxelVolumeGraph* Graph,
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
		Stamp = FVoxelVolumeGraphStampRef::New();
		Stamp->Graph = Graph;
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
	}

	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Volume Graph", DisplayName = "Break Voxel Volume Graph Stamp", meta = (Keywords = "Break"))
	static void Break(
		const FVoxelVolumeGraphStampRef Stamp,
		UPARAM(meta = (DisplayName = "Graph", ToolTip = "")) UVoxelVolumeGraph*& Graph,
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
		Graph = FVoxelUtilities::MakeSafe<UVoxelVolumeGraph*>();
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

		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Graph = Stamp->Graph;
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
	}

	// Get Graph
	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Volume Graph", DisplayName = "Get Graph")
	static void GetGraph(UPARAM(Required) FVoxelVolumeGraphStampRef Stamp, UVoxelVolumeGraph*& Graph)
	{
		Graph = FVoxelUtilities::MakeSafe<UVoxelVolumeGraph*>();

		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Graph = Stamp->Graph;
	}

	// Set Graph
	// This will automatically refresh the stamp
	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Volume Graph", DisplayName = "Set Graph")
	static void SetGraph(UPARAM(Required) FVoxelVolumeGraphStampRef Stamp, UPARAM(meta = (DisplayName = "Stamp")) FVoxelVolumeGraphStampRef& OutStamp, UVoxelVolumeGraph* Graph)
	{
		OutStamp = Stamp;
		
		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Stamp->Graph = Graph;
		Stamp.Update();
	}
};