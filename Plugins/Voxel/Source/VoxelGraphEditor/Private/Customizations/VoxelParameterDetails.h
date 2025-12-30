// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelPinType.h"

class FVoxelParameterView;
class FVoxelGraphEnvironment;
class FVoxelParameterOverridesDetails;

class FVoxelParameterDetails : public TSharedFromThis<FVoxelParameterDetails>
{
public:
	FVoxelParameterOverridesDetails& OverridesDetails;
	const FGuid Guid;
	const TVoxelArray<FVoxelParameterView*> ParameterViews;
	FName OrphanName;
	FVoxelPinType OrphanExposedType;

	FVoxelPinType RowExposedType;

	FVoxelParameterDetails(
		FVoxelParameterOverridesDetails& ContainerDetail,
		const FGuid& Guid,
		const TVoxelArray<FVoxelParameterView*>& ParameterViews);

	void InitializeOrphan(
		const FVoxelPinValue& Value,
		bool bNewHasSingleValue);

	void ComputeEditorGraphs(
		FVoxelDependencyCollector& DependencyCollector,
		const TVoxelArray<TSharedRef<FVoxelGraphEnvironment>>& Environments);

public:
	bool IsValid() const
	{
		return
			ensure(PropertyHandle) &&
			PropertyHandle->IsValidHandle();
	}
	bool IsOrphan() const
	{
		return ParameterViews.Num() == 0;
	}
	bool HasSingleValue() const
	{
		return bHasSingleValue;
	}

public:
	void Tick();

public:
	void MakeRow(const FVoxelDetailInterface& DetailInterface);

	void BuildRow(
		FDetailWidgetRow& Row,
		const TSharedRef<SWidget>& ValueWidget);

public:
	ECheckBoxState IsEnabled() const;
	void SetEnabled(bool bNewEnabled) const;

public:
	bool CanResetToDefault() const;
	void ResetToDefault();

public:
	void PreEditChange() const;
	void PostEditChange() const;

private:
	const TSharedRef<TStructOnScope<FVoxelPinValue>> StructOnScope = MakeShared<TStructOnScope<FVoxelPinValue>>();

	double LastSyncTime = 0.;
	bool bHasSingleValue = false;
	bool bForceEnableOverride = false;
	TSharedPtr<IPropertyHandle> PropertyHandle;
	TSharedPtr<FVoxelInstancedStructDetailsWrapper> StructWrapper;

	bool bIsVisible = true;
	bool bIsReadOnly = false;
	FString DisplayName;

	void SyncFromViews();
	FString GetRowName() const;
	FVoxelPinValue& GetValueRef() const;
};