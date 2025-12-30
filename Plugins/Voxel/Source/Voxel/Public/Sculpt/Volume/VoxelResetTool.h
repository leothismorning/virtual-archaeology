// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Sculpt/VoxelTool.h"
#include "VoxelResetTool.generated.h"

class AStaticMeshActor;

UCLASS(DisplayName = "Reset", meta = (Order = 4, Icon = "LandscapeEditor.FlattenTool"))
class VOXEL_API UVoxelResetTool : public UVoxelTool
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float Radius = 1000.f;

	//~ Begin UVoxelTool Interface
	virtual void Enter() override;
	virtual void Exit() override;
	virtual void Tick() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	//virtual void Edit(const TSharedRef<const FVoxelConfig>& Config) override; TODO
	//~ End UVoxelTool Interface

private:
	UPROPERTY()
	TObjectPtr<AStaticMeshActor> PreviewActor;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> PreviewMaterial;
};