// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelWeakStampRef.h"
#include "VoxelStampBehavior.h"
#include "VoxelMetadataOverrides.h"
#include "VoxelStamp.generated.h"

struct FVoxelStampRuntime;
class UVoxelStampComponent;
class IVoxelStampComponentInterface;

USTRUCT(meta = (Abstract))
struct VOXEL_API FVoxelStamp
	: public FVoxelVirtualStruct
	, public TSharedFromThis<FVoxelStamp>
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()
	VOXEL_COUNT_INSTANCES();

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	FTransform Transform;

	UPROPERTY(EditAnywhere, Category = "Config")
	EVoxelStampBehavior Behavior = EVoxelStampBehavior::AffectAll;

	// Priority of the stamp within its layer
	// Higher priority stamps will be applied last
	UPROPERTY(EditAnywhere, Category = "Config")
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, Category = "Config", meta = (Units = cm, ClampMin = 0))
	float Smoothness = 100;

	UPROPERTY(EditAnywhere, Category = "Config")
	FVoxelMetadataOverrides MetadataOverrides;

	// This stamp will only be applied on LODs within this range (inclusive)
	UPROPERTY(EditAnywhere, Category = "Config", DisplayName = "LOD Range", AdvancedDisplay)
	FInt32Interval LODRange = { 0, 32 };

	// If true you won't be able to select this stamp by clicking on it
	UPROPERTY(EditAnywhere, Category = "Config", AdvancedDisplay)
	bool bDisableStampSelection = false;

	// If false, this stamp will only apply on parts where another stamp has been applied first
	// This is useful to avoid having stamps going beyond world bounds
	// Only used if BlendMode is not Override nor Intersect
	UPROPERTY(EditAnywhere, Category = "Config", AdvancedDisplay)
	bool bApplyOnVoid = true;

	// By how much to extend the bounds, relative to the bounds size
	// Increase this if you are using a high smoothness
	// Increasing this will lead to more stamps being sampled per voxel, increasing generation cost
	UPROPERTY(EditAnywhere, Category = "Config", AdvancedDisplay, meta = (UIMin = 0, UIMax = 5))
	float BoundsExtension = 1.f;

public:
	virtual void FixupProperties(const IVoxelStampComponentInterface* Interface);

	virtual void PostDuplicate()
	{
	}
	virtual TVoxelArray<TSubclassOf<USceneComponent>> GetRequiredComponents() const
	{
		return {};
	}

public:
#if WITH_EDITOR
	struct IPreview
	{
		virtual ~IPreview() = default;
		virtual USceneComponent* GetComponent(UClass* Class) = 0;

		template<typename T>
		T* GetComponent()
		{
			return CastChecked<T>(this->GetComponent(StaticClassFast<T>()), ECastCheckedType::NullAllowed);
		}
	};
	virtual void SetupPreview(IPreview& Preview) const {}

	struct FActorLabelInfo
	{
		FString BlendMode;
		FString Identifier;
	};
	virtual void GetActorLabelInfo(FActorLabelInfo& Info) const
	{
	}

	struct FPropertyInfo
	{
		bool bIsBlendModeVisible = true;
		bool bIsSmoothnessVisible = true;
		bool bIsMetadataOverridesVisible = true;

		FPropertyInfo& operator&=(const FPropertyInfo& Other)
		{
			bIsBlendModeVisible &= Other.bIsBlendModeVisible;
			bIsSmoothnessVisible &= Other.bIsSmoothnessVisible;
			bIsMetadataOverridesVisible &= Other.bIsMetadataOverridesVisible;
			return *this;
		}
	};
	virtual void GetPropertyInfo(FPropertyInfo& Info) const
	{
	}

	FActorLabelInfo GetActorLabelInfo() const;
#endif

public:
	FVoxelStampRef GetStampRef() const;
	TVoxelObjectPtr<USceneComponent> GetComponent() const;
	TSharedPtr<const FVoxelStampRuntime> ResolveStampRuntime() const;

private:
	FVoxelWeakStampRef WeakStampRef;

	friend FVoxelStampRef;
};