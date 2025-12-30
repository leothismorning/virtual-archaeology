// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelVolumeSculptStampRef.h"
#include "VoxelStampBlueprintFunctionLibrary.h"
#include "VoxelVolumeSculptStamp_K2.generated.h"

////////////////////////////////////////////////////
///////// The code below is auto-generated /////////
////////////////////////////////////////////////////

UCLASS()
class VOXEL_API UVoxelVolumeSculptStamp_K2 : public UVoxelStampBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Casting", meta = (ExpandEnumAsExecs = "Result"))
	static FVoxelVolumeSculptStampRef CastToVolumeSculptStamp(const FVoxelStampRef Stamp, EVoxelStampCastResult& Result)
	{
		return CastToStampImpl<FVoxelVolumeSculptStamp>(Stamp, Result);
	}

	// Make a copy of this stamp
	// You can then call Set XXXX on the copy without having the original stamp be modified
	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Volume Sculpt", DisplayName = "Get Volume Sculpt Stamp")
	static void MakeCopy(const FVoxelVolumeSculptStampRef Stamp, FVoxelVolumeSculptStampRef& Copy)
	{
		Copy = FVoxelVolumeSculptStampRef(Stamp.MakeCopy());
	}

	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Volume Sculpt", DisplayName = "Make Voxel Volume Sculpt Stamp", meta = (Keywords = "Construct, Create", Scale = "100.000000", bUseFastDistances = "false", bEnableDiffing = "true", Layer = "/Voxel/Default/DefaultVolumeLayer.DefaultVolumeLayer", BlendMode = "Override", Transform = "0.000000,0.000000,0.000000|0.000000,0.000000,-0.000000|1.000000,1.000000,1.000000", Behavior = "AffectAll", Priority = "0", Smoothness = "100.000000", MetadataOverrides = "(Overrides=)", LODRange = "(Min=0,Max=32)", bDisableStampSelection = "false", BoundsExtension = "1.000000", AutoCreateRefTerm = "AdditionalLayers"))
	static void Make(
		FVoxelVolumeSculptStampRef& Stamp,
		UPARAM(meta = (DisplayName = "Scale", ToolTip = "")) float Scale,
		UPARAM(meta = (DisplayName = "Use Fast Distances", ToolTip = "If true will compress distances to one byte\nSetting this will clear any existing data")) bool bUseFastDistances,
		UPARAM(meta = (DisplayName = "Enable Diffing", ToolTip = "If false, edits won\'t be diffed\nThis make editing up to 5x faster, but will lead to obvious chunks if you move the underlying stamps after editing")) bool bEnableDiffing,
		UPARAM(meta = (DisplayName = "Stack Override", ToolTip = "Use this if this stamp is not rendered in the Voxel World stack\nThis stack will be used during sculpting to query the distances before any sculpt is applied")) UVoxelLayerStack* StackOverride,
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
		UPARAM(meta = (DisplayName = "Bounds Extension", ToolTip = "By how much to extend the bounds, relative to the bounds size\nIncrease this if you are using a high smoothness\nIncreasing this will lead to more stamps being sampled per voxel, increasing generation cost")) float BoundsExtension)
	{
		Stamp = FVoxelVolumeSculptStampRef::New();
		Stamp->Scale = Scale;
		Stamp->bUseFastDistances = bUseFastDistances;
		Stamp->bEnableDiffing = bEnableDiffing;
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

	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Volume Sculpt", DisplayName = "Break Voxel Volume Sculpt Stamp", meta = (Keywords = "Break"))
	static void Break(
		const FVoxelVolumeSculptStampRef Stamp,
		UPARAM(meta = (DisplayName = "Scale", ToolTip = "")) float& Scale,
		UPARAM(meta = (DisplayName = "Use Fast Distances", ToolTip = "If true will compress distances to one byte\nSetting this will clear any existing data")) bool& bUseFastDistances,
		UPARAM(meta = (DisplayName = "Enable Diffing", ToolTip = "If false, edits won\'t be diffed\nThis make editing up to 5x faster, but will lead to obvious chunks if you move the underlying stamps after editing")) bool& bEnableDiffing,
		UPARAM(meta = (DisplayName = "Stack Override", ToolTip = "Use this if this stamp is not rendered in the Voxel World stack\nThis stack will be used during sculpting to query the distances before any sculpt is applied")) UVoxelLayerStack*& StackOverride,
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
		UPARAM(meta = (DisplayName = "Bounds Extension", ToolTip = "By how much to extend the bounds, relative to the bounds size\nIncrease this if you are using a high smoothness\nIncreasing this will lead to more stamps being sampled per voxel, increasing generation cost")) float& BoundsExtension)
	{
		Scale = FVoxelUtilities::MakeSafe<float>();
		bUseFastDistances = FVoxelUtilities::MakeSafe<bool>();
		bEnableDiffing = FVoxelUtilities::MakeSafe<bool>();
		StackOverride = FVoxelUtilities::MakeSafe<UVoxelLayerStack*>();
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
		BoundsExtension = FVoxelUtilities::MakeSafe<float>();

		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Scale = Stamp->Scale;
		bUseFastDistances = Stamp->bUseFastDistances;
		bEnableDiffing = Stamp->bEnableDiffing;
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

	// Get Scale
	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Volume Sculpt", DisplayName = "Get Scale")
	static void GetScale(UPARAM(Required) FVoxelVolumeSculptStampRef Stamp, float& Scale)
	{
		Scale = FVoxelUtilities::MakeSafe<float>();

		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Scale = Stamp->Scale;
	}

	// Set Scale
	// This will automatically refresh the stamp
	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Volume Sculpt", DisplayName = "Set Scale")
	static void SetScale(UPARAM(Required) FVoxelVolumeSculptStampRef Stamp, UPARAM(meta = (DisplayName = "Stamp")) FVoxelVolumeSculptStampRef& OutStamp, float Scale)
	{
		OutStamp = Stamp;
		
		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Stamp->Scale = Scale;
		Stamp.Update();
	}

	// Get Use Fast Distances
	//
	// If true will compress distances to one byte
	// Setting this will clear any existing data
	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Volume Sculpt", DisplayName = "Get Use Fast Distances")
	static void GetUseFastDistances(UPARAM(Required) FVoxelVolumeSculptStampRef Stamp, bool& bUseFastDistances)
	{
		bUseFastDistances = FVoxelUtilities::MakeSafe<bool>();

		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		bUseFastDistances = Stamp->bUseFastDistances;
	}

	// Set Use Fast Distances
	// This will automatically refresh the stamp
	//
	// If true will compress distances to one byte
	// Setting this will clear any existing data
	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Volume Sculpt", DisplayName = "Set Use Fast Distances")
	static void SetUseFastDistances(UPARAM(Required) FVoxelVolumeSculptStampRef Stamp, UPARAM(meta = (DisplayName = "Stamp")) FVoxelVolumeSculptStampRef& OutStamp, bool bUseFastDistances)
	{
		OutStamp = Stamp;
		
		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Stamp->bUseFastDistances = bUseFastDistances;
		Stamp.Update();
	}

	// Get Enable Diffing
	//
	// If false, edits won't be diffed
	// This make editing up to 5x faster, but will lead to obvious chunks if you move the underlying stamps after editing
	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Volume Sculpt", DisplayName = "Get Enable Diffing")
	static void GetEnableDiffing(UPARAM(Required) FVoxelVolumeSculptStampRef Stamp, bool& bEnableDiffing)
	{
		bEnableDiffing = FVoxelUtilities::MakeSafe<bool>();

		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		bEnableDiffing = Stamp->bEnableDiffing;
	}

	// Set Enable Diffing
	// This will automatically refresh the stamp
	//
	// If false, edits won't be diffed
	// This make editing up to 5x faster, but will lead to obvious chunks if you move the underlying stamps after editing
	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Volume Sculpt", DisplayName = "Set Enable Diffing")
	static void SetEnableDiffing(UPARAM(Required) FVoxelVolumeSculptStampRef Stamp, UPARAM(meta = (DisplayName = "Stamp")) FVoxelVolumeSculptStampRef& OutStamp, bool bEnableDiffing)
	{
		OutStamp = Stamp;
		
		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Stamp->bEnableDiffing = bEnableDiffing;
		Stamp.Update();
	}

	// Get Stack Override
	//
	// Use this if this stamp is not rendered in the Voxel World stack
	// This stack will be used during sculpting to query the distances before any sculpt is applied
	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Volume Sculpt", DisplayName = "Get Stack Override")
	static void GetStackOverride(UPARAM(Required) FVoxelVolumeSculptStampRef Stamp, UVoxelLayerStack*& StackOverride)
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
	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Volume Sculpt", DisplayName = "Set Stack Override")
	static void SetStackOverride(UPARAM(Required) FVoxelVolumeSculptStampRef Stamp, UPARAM(meta = (DisplayName = "Stamp")) FVoxelVolumeSculptStampRef& OutStamp, UVoxelLayerStack* StackOverride)
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