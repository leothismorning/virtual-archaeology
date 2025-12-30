// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelHeightmapStampRef.h"
#include "VoxelStampBlueprintFunctionLibrary.h"
#include "VoxelHeightmapStamp_K2.generated.h"

////////////////////////////////////////////////////
///////// The code below is auto-generated /////////
////////////////////////////////////////////////////

UCLASS()
class VOXEL_API UVoxelHeightmapStamp_K2 : public UVoxelStampBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Casting", meta = (ExpandEnumAsExecs = "Result"))
	static FVoxelHeightmapStampRef CastToHeightmapStamp(const FVoxelStampRef Stamp, EVoxelStampCastResult& Result)
	{
		return CastToStampImpl<FVoxelHeightmapStamp>(Stamp, Result);
	}

	// Make a copy of this stamp
	// You can then call Set XXXX on the copy without having the original stamp be modified
	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Heightmap", DisplayName = "Get Heightmap Stamp")
	static void MakeCopy(const FVoxelHeightmapStampRef Stamp, FVoxelHeightmapStampRef& Copy)
	{
		Copy = FVoxelHeightmapStampRef(Stamp.MakeCopy());
	}

	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Heightmap", DisplayName = "Make Voxel Heightmap Stamp", meta = (Keywords = "Construct, Create", Layer = "/Voxel/Default/DefaultHeightLayer.DefaultHeightLayer", BlendMode = "Max", Transform = "0.000000,0.000000,0.000000|0.000000,0.000000,-0.000000|1.000000,1.000000,1.000000", Behavior = "AffectAll", Priority = "0", Smoothness = "100.000000", MetadataOverrides = "(Overrides=)", LODRange = "(Min=0,Max=32)", bDisableStampSelection = "false", bApplyOnVoid = "true", BoundsExtension = "0.100000", AutoCreateRefTerm = "WeightmapSurfaceTypes, AdditionalLayers"))
	static void Make(
		FVoxelHeightmapStampRef& Stamp,
		UPARAM(meta = (DisplayName = "Heightmap", ToolTip = "")) UVoxelHeightmap* Heightmap,
		UPARAM(meta = (DisplayName = "Default Surface Type", ToolTip = "Default surface used as base when no weightmap is applied")) UVoxelSurfaceTypeInterface* DefaultSurfaceType,
		UPARAM(meta = (DisplayName = "Weightmap Surface Types", ToolTip = "Use this to override weightmap surfaces")) TArray<FVoxelHeightmapStampWeightmapSurfaceType> WeightmapSurfaceTypes,
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
		UPARAM(meta = (DisplayName = "Apply on Void", ToolTip = "If false, this stamp will only apply on parts where another stamp has been applied first\nThis is useful to avoid having stamps going beyond world bounds\nOnly used if BlendMode is not Override nor Intersect")) bool bApplyOnVoid,
		UPARAM(meta = (DisplayName = "Bounds Extension", ToolTip = "By how much to extend the bounds, relative to the bounds size\nIncrease this if you are using a high smoothness\nIncreasing this will lead to more stamps being sampled per voxel, increasing generation cost")) float BoundsExtension)
	{
		Stamp = FVoxelHeightmapStampRef::New();
		Stamp->Heightmap = Heightmap;
		Stamp->DefaultSurfaceType = DefaultSurfaceType;
		Stamp->WeightmapSurfaceTypes = WeightmapSurfaceTypes;
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

	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Heightmap", DisplayName = "Break Voxel Heightmap Stamp", meta = (Keywords = "Break"))
	static void Break(
		const FVoxelHeightmapStampRef Stamp,
		UPARAM(meta = (DisplayName = "Heightmap", ToolTip = "")) UVoxelHeightmap*& Heightmap,
		UPARAM(meta = (DisplayName = "Default Surface Type", ToolTip = "Default surface used as base when no weightmap is applied")) UVoxelSurfaceTypeInterface*& DefaultSurfaceType,
		UPARAM(meta = (DisplayName = "Weightmap Surface Types", ToolTip = "Use this to override weightmap surfaces")) TArray<FVoxelHeightmapStampWeightmapSurfaceType>& WeightmapSurfaceTypes,
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
		UPARAM(meta = (DisplayName = "Apply on Void", ToolTip = "If false, this stamp will only apply on parts where another stamp has been applied first\nThis is useful to avoid having stamps going beyond world bounds\nOnly used if BlendMode is not Override nor Intersect")) bool& bApplyOnVoid,
		UPARAM(meta = (DisplayName = "Bounds Extension", ToolTip = "By how much to extend the bounds, relative to the bounds size\nIncrease this if you are using a high smoothness\nIncreasing this will lead to more stamps being sampled per voxel, increasing generation cost")) float& BoundsExtension)
	{
		Heightmap = FVoxelUtilities::MakeSafe<UVoxelHeightmap*>();
		DefaultSurfaceType = FVoxelUtilities::MakeSafe<UVoxelSurfaceTypeInterface*>();
		WeightmapSurfaceTypes = FVoxelUtilities::MakeSafe<TArray<FVoxelHeightmapStampWeightmapSurfaceType>>();
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
		bApplyOnVoid = FVoxelUtilities::MakeSafe<bool>();
		BoundsExtension = FVoxelUtilities::MakeSafe<float>();

		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Heightmap = Stamp->Heightmap;
		DefaultSurfaceType = Stamp->DefaultSurfaceType;
		WeightmapSurfaceTypes = Stamp->WeightmapSurfaceTypes;
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

	// Get Heightmap
	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Heightmap", DisplayName = "Get Heightmap")
	static void GetHeightmap(UPARAM(Required) FVoxelHeightmapStampRef Stamp, UVoxelHeightmap*& Heightmap)
	{
		Heightmap = FVoxelUtilities::MakeSafe<UVoxelHeightmap*>();

		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Heightmap = Stamp->Heightmap;
	}

	// Set Heightmap
	// This will automatically refresh the stamp
	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Heightmap", DisplayName = "Set Heightmap")
	static void SetHeightmap(UPARAM(Required) FVoxelHeightmapStampRef Stamp, UPARAM(meta = (DisplayName = "Stamp")) FVoxelHeightmapStampRef& OutStamp, UVoxelHeightmap* Heightmap)
	{
		OutStamp = Stamp;
		
		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Stamp->Heightmap = Heightmap;
		Stamp.Update();
	}

	// Get Default Surface Type
	//
	// Default surface used as base when no weightmap is applied
	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Heightmap", DisplayName = "Get Default Surface Type")
	static void GetDefaultSurfaceType(UPARAM(Required) FVoxelHeightmapStampRef Stamp, UVoxelSurfaceTypeInterface*& DefaultSurfaceType)
	{
		DefaultSurfaceType = FVoxelUtilities::MakeSafe<UVoxelSurfaceTypeInterface*>();

		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		DefaultSurfaceType = Stamp->DefaultSurfaceType;
	}

	// Set Default Surface Type
	// This will automatically refresh the stamp
	//
	// Default surface used as base when no weightmap is applied
	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Heightmap", DisplayName = "Set Default Surface Type")
	static void SetDefaultSurfaceType(UPARAM(Required) FVoxelHeightmapStampRef Stamp, UPARAM(meta = (DisplayName = "Stamp")) FVoxelHeightmapStampRef& OutStamp, UVoxelSurfaceTypeInterface* DefaultSurfaceType)
	{
		OutStamp = Stamp;
		
		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Stamp->DefaultSurfaceType = DefaultSurfaceType;
		Stamp.Update();
	}

	// Get Weightmap Surface Types
	//
	// Use this to override weightmap surfaces
	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Heightmap", DisplayName = "Get Weightmap Surface Types")
	static void GetWeightmapSurfaceTypes(UPARAM(Required) FVoxelHeightmapStampRef Stamp, TArray<FVoxelHeightmapStampWeightmapSurfaceType>& WeightmapSurfaceTypes)
	{
		WeightmapSurfaceTypes = FVoxelUtilities::MakeSafe<TArray<FVoxelHeightmapStampWeightmapSurfaceType>>();

		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		WeightmapSurfaceTypes = Stamp->WeightmapSurfaceTypes;
	}

	// Set Weightmap Surface Types
	// This will automatically refresh the stamp
	//
	// Use this to override weightmap surfaces
	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Heightmap", DisplayName = "Set Weightmap Surface Types")
	static void SetWeightmapSurfaceTypes(UPARAM(Required) FVoxelHeightmapStampRef Stamp, UPARAM(meta = (DisplayName = "Stamp")) FVoxelHeightmapStampRef& OutStamp, TArray<FVoxelHeightmapStampWeightmapSurfaceType> WeightmapSurfaceTypes)
	{
		OutStamp = Stamp;
		
		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Stamp->WeightmapSurfaceTypes = WeightmapSurfaceTypes;
		Stamp.Update();
	}
};