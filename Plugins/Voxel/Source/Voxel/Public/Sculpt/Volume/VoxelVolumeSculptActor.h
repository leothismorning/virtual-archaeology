// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelStampComponentBase.h"
#include "Sculpt/VoxelSculptActorBase.h"
#include "Sculpt/Volume/VoxelVolumeSculptStampRef.h"
#include "VoxelVolumeSculptActor.generated.h"

struct FVoxelVolumeSculptSave;

UCLASS()
class VOXEL_API UVoxelVolumeSculptComponent : public UVoxelStampComponentBase
{
	GENERATED_BODY()

public:
	FVoxelVolumeSculptStampRef GetStamp() const;

	//~ Begin UVoxelStampComponentBase Interface
	virtual void Serialize(FArchive& Ar) override;
	virtual FVoxelStampRef GetStamp_Internal() const override;
	//~ End UVoxelStampComponentBase Interface

private:
	UPROPERTY(EditAnywhere, Category = "Config", DisplayName = "Stamp", meta = (ShowOnlyInnerProperties, HideTransform))
	FVoxelVolumeSculptStampRef PrivateStamp;

	friend class UVoxelStampComponent;
};

UCLASS()
class VOXEL_API AVoxelVolumeSculptActor : public AVoxelSculptActorBase
{
	GENERATED_BODY()

public:
	AVoxelVolumeSculptActor();

public:
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	FVoxelVolumeSculptStampRef GetStamp() const;

public:
	FVoxelFuture ApplyModifier(const TSharedRef<FVoxelVolumeModifier>& Modifier);
	FVoxelFuture ClearSculptData();
	virtual void ClearSculptCache() override;

public:
	TVoxelFuture<FVoxelVolumeSculptSave> GetSave(bool bCompress = true) const;
	FVoxelFuture LoadFromSave(const FVoxelVolumeSculptSave& Save);

private:
	UPROPERTY(VisibleAnywhere, Category = "Config")
	TObjectPtr<UVoxelVolumeSculptComponent> Component;
};