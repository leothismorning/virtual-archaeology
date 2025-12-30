// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelHeightStampRef.h"
#include "VoxelStampBlueprintFunctionLibrary.h"
#include "VoxelHeightStamp_K2.generated.h"

////////////////////////////////////////////////////
///////// The code below is auto-generated /////////
////////////////////////////////////////////////////

UCLASS()
class VOXEL_API UVoxelHeightStamp_K2 : public UVoxelStampBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Casting", meta = (ExpandEnumAsExecs = "Result"))
	static FVoxelHeightStampRef CastToHeightStamp(const FVoxelStampRef Stamp, EVoxelStampCastResult& Result)
	{
		return CastToStampImpl<FVoxelHeightStamp>(Stamp, Result);
	}

	// Make a copy of this stamp
	// You can then call Set XXXX on the copy without having the original stamp be modified
	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Height", DisplayName = "Get Height Stamp")
	static void MakeCopy(const FVoxelHeightStampRef Stamp, FVoxelHeightStampRef& Copy)
	{
		Copy = FVoxelHeightStampRef(Stamp.MakeCopy());
	}

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void Make(FVoxelHeightStampRef& Stamp)
	{
		Stamp = {};
	}

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void Break(const FVoxelHeightStampRef Stamp)
	{
	}

	// Get Layer
	//
	// Layer that this stamps belong to
	// You can control the order of layers in Layer Stacks
	// You can select the layer stack to use in your Voxel World or PCG Sampler settings
	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Height", DisplayName = "Get Layer")
	static void GetLayer(UPARAM(Required) FVoxelHeightStampRef Stamp, UVoxelHeightLayer*& Layer)
	{
		Layer = FVoxelUtilities::MakeSafe<UVoxelHeightLayer*>();

		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Layer = Stamp->Layer;
	}

	// Set Layer
	// This will automatically refresh the stamp
	//
	// Layer that this stamps belong to
	// You can control the order of layers in Layer Stacks
	// You can select the layer stack to use in your Voxel World or PCG Sampler settings
	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Height", DisplayName = "Set Layer")
	static void SetLayer(UPARAM(Required) FVoxelHeightStampRef Stamp, UPARAM(meta = (DisplayName = "Stamp")) FVoxelHeightStampRef& OutStamp, UVoxelHeightLayer* Layer)
	{
		OutStamp = Stamp;
		
		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Stamp->Layer = Layer;
		Stamp.Update();
	}

	// Get Blend Mode
	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Height", DisplayName = "Get Blend Mode")
	static void GetBlendMode(UPARAM(Required) FVoxelHeightStampRef Stamp, EVoxelHeightBlendMode& BlendMode)
	{
		BlendMode = FVoxelUtilities::MakeSafe<EVoxelHeightBlendMode>();

		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		BlendMode = Stamp->BlendMode;
	}

	// Set Blend Mode
	// This will automatically refresh the stamp
	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Height", DisplayName = "Set Blend Mode")
	static void SetBlendMode(UPARAM(Required) FVoxelHeightStampRef Stamp, UPARAM(meta = (DisplayName = "Stamp")) FVoxelHeightStampRef& OutStamp, EVoxelHeightBlendMode BlendMode)
	{
		OutStamp = Stamp;
		
		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Stamp->BlendMode = BlendMode;
		Stamp.Update();
	}

	// Get Additional Layers
	UFUNCTION(BlueprintPure, Category = "Voxel|Stamp|Height", DisplayName = "Get Additional Layers")
	static void GetAdditionalLayers(UPARAM(Required) FVoxelHeightStampRef Stamp, TArray<UVoxelHeightLayer*>& AdditionalLayers)
	{
		AdditionalLayers = FVoxelUtilities::MakeSafe<TArray<UVoxelHeightLayer*>>();

		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		AdditionalLayers = Stamp->AdditionalLayers;
	}

	// Set Additional Layers
	// This will automatically refresh the stamp
	UFUNCTION(BlueprintCallable, Category = "Voxel|Stamp|Height", DisplayName = "Set Additional Layers")
	static void SetAdditionalLayers(UPARAM(Required) FVoxelHeightStampRef Stamp, UPARAM(meta = (DisplayName = "Stamp")) FVoxelHeightStampRef& OutStamp, TArray<UVoxelHeightLayer*> AdditionalLayers)
	{
		OutStamp = Stamp;
		
		if (!Stamp.IsValid())
		{
			VOXEL_MESSAGE(Error, "Stamp is invalid");
			return;
		}

		Stamp->AdditionalLayers = AdditionalLayers;
		Stamp.Update();
	}
};