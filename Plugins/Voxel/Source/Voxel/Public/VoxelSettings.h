// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelDeveloperSettings.h"
#include "VoxelLayer.h"
#include "VoxelSettings.generated.h"

UCLASS(config = Engine, DefaultConfig, meta = (DisplayName = "Voxel Plugin"))
class VOXEL_API UVoxelSettings : public UVoxelDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "Config")
	bool bEnableAutoStampActorLabeling = true;

	// Voxel Stamp Actor label format
	// Type - Stamp Type;
	// Identifier - Name of related object (mesh stamp - mesh name, graph stamp - graph name);
	// BlendMode;
	// Layer - First layer name;
	UPROPERTY(Config, EditAnywhere, Category = "Config", meta = (EditCondition = "bEnableAutoStampActorLabeling"))
	FString StampActorLabelFormat = "{Identifier} {BlendMode} {Layer} {Type}";

	UPROPERTY(Config, EditAnywhere, Category = "Layers")
	TSoftObjectPtr<UVoxelHeightLayer> DefaultHeightLayer = UVoxelHeightLayer::Default();

	UPROPERTY(Config, EditAnywhere, Category = "Layers")
	TSoftObjectPtr<UVoxelVolumeLayer> DefaultVolumeLayer = UVoxelVolumeLayer::Default();

	UVoxelSettings();
};