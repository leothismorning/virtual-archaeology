// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelStampComponentBase.h"
#include "VoxelStampComponentInterface.h"
#include "VoxelStampComponent.generated.h"

UCLASS(meta = (VoxelPreviewComponent))
class VOXEL_API UVoxelStampDummyPreviewComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UVoxelStampDummyPreviewComponent();

	//~ Begin USceneComponent Interface
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ End USceneComponent Interface
};

UCLASS(BlueprintType, ClassGroup = Voxel, meta = (BlueprintSpawnableComponent))
class VOXEL_API UVoxelStampComponent
	: public UVoxelStampComponentBase
	, public IVoxelStampComponentInterface
{
	GENERATED_BODY()

public:
	virtual ~UVoxelStampComponent() override;

	//~ Begin USceneComponent Interface
	virtual void OnRegister() override;
	virtual void Serialize(FArchive& Ar) override;
#if WITH_EDITOR
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
#endif
	//~ End USceneComponent Interface

public:
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	FORCEINLINE FVoxelStampRef GetStamp() const
	{
		return PrivateStamp;
	}

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void SetStamp(const FVoxelStampRef& NewStamp);
	void SetStamp(const FVoxelStamp& NewStamp);

public:
	//~ Begin IVoxelStampComponentInterface Interface
	virtual USceneComponent* FindComponent(UClass* Class) const override;
	//~ End IVoxelStampComponentInterface Interface

#if WITH_EDITOR
	void OnActorLabelChanged();
#endif

private:
	UPROPERTY(EditAnywhere, Category = "Config", DisplayName = "Stamp", meta = (ShowOnlyInnerProperties, HideTransform, SplitStampProperties))
	FVoxelStampRef PrivateStamp;

	UPROPERTY()
	TSet<TObjectPtr<USceneComponent>> ChildComponents;

	UPROPERTY(Transient)
	TSet<TObjectPtr<USceneComponent>> OrphanChildComponents;

	//~ Begin UVoxelStampComponentBase Interface
	virtual void PreUpdateStamp() override;
	virtual FVoxelStampRef GetStamp_Internal() const override;
	//~ End UVoxelStampComponentBase Interface

#if WITH_EDITOR
	void UpdatePreview();
	void UpdateStampActorLabel() const;
	void UpdateRequiredComponents();
#endif

#if WITH_EDITOR
	FString GetStampActorLabel() const;
#endif
};