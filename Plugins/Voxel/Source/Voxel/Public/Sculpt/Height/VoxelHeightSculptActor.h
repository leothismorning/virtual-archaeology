// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelStampComponentBase.h"
#include "Sculpt/VoxelSculptActorBase.h"
#include "Sculpt/Height/VoxelHeightSculptStampRef.h"
#include "VoxelHeightSculptActor.generated.h"

struct FVoxelHeightSculptSave;

UCLASS()
class VOXEL_API UVoxelHeightSculptComponent : public UVoxelStampComponentBase
{
	GENERATED_BODY()

public:
	FVoxelHeightSculptStampRef GetStamp() const;

	//~ Begin UVoxelStampComponentBase Interface
	virtual void Serialize(FArchive& Ar) override;
	virtual FVoxelStampRef GetStamp_Internal() const override;
	//~ End UVoxelStampComponentBase Interface

private:
	UPROPERTY(EditAnywhere, Category = "Config", DisplayName = "Stamp", meta = (ShowOnlyInnerProperties, HideTransform))
	FVoxelHeightSculptStampRef PrivateStamp;

	friend class UVoxelStampComponent;
};

UCLASS()
class VOXEL_API AVoxelHeightSculptActor : public AVoxelSculptActorBase
{
	GENERATED_BODY()

public:
	AVoxelHeightSculptActor();

public:
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	FVoxelHeightSculptStampRef GetStamp() const;

public:
	FVoxelFuture ApplyModifier(const TSharedRef<FVoxelHeightModifier>& Modifier);
	FVoxelFuture ClearSculptData();
	virtual void ClearSculptCache() override;

public:
	TVoxelFuture<FVoxelHeightSculptSave> GetSave(bool bCompress = true) const;
	FVoxelFuture LoadFromSave(const FVoxelHeightSculptSave& Save);

private:
	UPROPERTY(VisibleAnywhere, Category = "Config")
	TObjectPtr<UVoxelHeightSculptComponent> Component;
};