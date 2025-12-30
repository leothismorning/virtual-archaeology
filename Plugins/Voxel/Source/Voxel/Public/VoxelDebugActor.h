// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelStackLayer.h"
#include "VoxelDebugActor.generated.h"

USTRUCT()
struct FVoxelDebugActorSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	int32 LOD = 0;

	UPROPERTY(EditAnywhere, Category = "Config")
	FVoxelStackLayer Layer;

	UPROPERTY(EditAnywhere, Category = "Config")
	int32 Size = 100;

	UPROPERTY(EditAnywhere, Category = "Config")
	float VoxelSize = 100;

	UPROPERTY(EditAnywhere, Category = "Config")
	bool bGrayscale = false;

	UPROPERTY(EditAnywhere, Category = "Config")
	float GrayscaleScale = 1.f;

	UPROPERTY(EditAnywhere, Category = "Config")
	float ColorStep = 100.f;

public:
	bool operator==(const FVoxelDebugActorSettings& Other) const
	{
		return
			LOD == Other.LOD &&
			Layer == Other.Layer &&
			Size == Other.Size &&
			VoxelSize == Other.VoxelSize &&
			bGrayscale == Other.bGrayscale &&
			GrayscaleScale == Other.GrayscaleScale &&
			ColorStep == Other.ColorStep;
	}
};

UCLASS(NotBlueprintable)
class VOXEL_API AVoxelDebugActor : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config", meta = (ShowOnlyInnerProperties))
	FVoxelDebugActorSettings Settings;

	UPROPERTY(EditAnywhere, Category = "Config", AdvancedDisplay)
	TObjectPtr<UMaterialInterface> ParentMaterial;

	AVoxelDebugActor();

	void Refresh();

	//~ Begin AActor Interface
	virtual void Serialize(FArchive& Ar) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
	//~ End AActor Interface

private:
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> Material;

	UPROPERTY(Transient, DuplicateTransient, NonTransactional)
	TArray<TObjectPtr<UTexture2D>> Textures;

	FVoxelFuture LastFuture;
	FTransform LastTransform = FTransform(FVector(FVoxelUtilities::NaNd()));
	FVoxelDebugActorSettings LastSettings;
	TSharedPtr<FVoxelDependencyTracker> DependencyTracker;
};