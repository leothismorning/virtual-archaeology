// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Customizations/VoxelParameterDetails.h"
#include "VoxelGraph.h"
#include "VoxelNodeEvaluator.h"
#include "VoxelParameterView.h"
#include "VoxelCompiledGraph.h"
#include "VoxelObjectPinType.h"
#include "VoxelCompiledTerminalGraph.h"
#include "VoxelInvalidationCallstack.h"
#include "VoxelParameterOverridesDetails.h"
#include "VoxelGraphParametersViewContext.h"
#include "VoxelPinValueCustomizationHelper.h"
#include "Nodes/VoxelNode_CustomizeParameter.h"

FVoxelParameterDetails::FVoxelParameterDetails(
	FVoxelParameterOverridesDetails& ContainerDetail,
	const FGuid& Guid,
	const TVoxelArray<FVoxelParameterView*>& ParameterViews)
	: OverridesDetails(ContainerDetail)
	, Guid(Guid)
	, ParameterViews(ParameterViews)
{
	StructOnScope->InitializeAs<FVoxelPinValue>();

	for (const FVoxelParameterOverridesDetails::FOwner& Owner : OverridesDetails.GetOwners())
	{
		bForceEnableOverride = Owner.Parameters.ShouldForceEnableOverride(Guid);
	}
	for (const FVoxelParameterOverridesDetails::FOwner& Owner : OverridesDetails.GetOwners())
	{
		ensure(bForceEnableOverride == Owner.Parameters.ShouldForceEnableOverride(Guid));
	}

	if (!IsOrphan())
	{
		SyncFromViews();
	}
}

void FVoxelParameterDetails::InitializeOrphan(
	const FVoxelPinValue& Value,
	const bool bNewHasSingleValue)
{
	ensure(IsOrphan());
	ensure(Value.IsValid());

	GetValueRef() = Value;
	bHasSingleValue = bNewHasSingleValue;
}

void FVoxelParameterDetails::ComputeEditorGraphs(
	FVoxelDependencyCollector& DependencyCollector,
	const TVoxelArray<TSharedRef<FVoxelGraphEnvironment>>& Environments)
{
	VOXEL_FUNCTION_COUNTER();

	if (IsOrphan())
	{
		return;
	}

	bIsVisible = true;
	bIsReadOnly = false;
	DisplayName = "";
	bool bMultipleDisplayNames = false;

	for (const TSharedRef<FVoxelGraphEnvironment>& Environment : Environments)
	{
		const UVoxelGraph* Graph = Environment->RootCompiledGraph->Graph.Resolve();
		if (!ensureVoxelSlow(Graph))
		{
			continue;
		}

		const TSharedRef<const FVoxelCompiledGraph> CompiledGraph = Graph->GetCompiledGraph(DependencyCollector);
		const FVoxelCompiledTerminalGraph* CompiledTerminalGraph = CompiledGraph->FindTerminalGraph(GVoxelEditorTerminalGraphGuid);
		if (!CompiledTerminalGraph)
		{
			continue;
		}

		const FVoxelNode_CustomizeParameter* Node = CompiledTerminalGraph->FindCustomizeParameterNode(Guid);
		if (!Node)
		{
			continue;
		}

		const TVoxelNodeEvaluator<FVoxelNode_CustomizeParameter> Evaluator = FVoxelNodeEvaluator::Create<FVoxelNode_CustomizeParameter>(
			Environment,
			Graph->GetEditorTerminalGraph(),
			*Node);

		if (!Evaluator)
		{
			continue;
		}

		FVoxelGraphContext Context = Evaluator.MakeContext(DependencyCollector);
		FVoxelGraphQueryImpl& Query = Context.MakeQuery();

		bIsVisible &= Evaluator->IsVisiblePin.GetSynchronous(Query);
		bIsReadOnly |= Evaluator->IsReadOnlyPin.GetSynchronous(Query);

		if (bMultipleDisplayNames)
		{
			continue;
		}

		FName Name = Evaluator->DisplayNamePin.GetSynchronous(Query);
		if (Name.IsNone())
		{
			continue;
		}

		FString NameString = Name.ToString();
		if (!DisplayName.IsEmpty() &&
			DisplayName != NameString)
		{
			DisplayName = {};
			bMultipleDisplayNames = true;
			continue;
		}

		DisplayName = NameString;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelParameterDetails::Tick()
{
	const double Time = FPlatformTime::Seconds();
	if (!IsOrphan() &&
		LastSyncTime + 0.1 < Time)
	{
		LastSyncTime = Time;
		SyncFromViews();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelParameterDetails::MakeRow(const FVoxelDetailInterface& DetailInterface)
{
	VOXEL_FUNCTION_COUNTER();

	if (!IsOrphan())
	{
		ensure(!RowExposedType.IsValid());
		RowExposedType = ParameterViews[0]->GetType().GetExposedType();
	}
	else
	{
		RowExposedType = OrphanExposedType;
	}

	IDetailPropertyRow* DummyRow = DetailInterface.AddExternalStructure(StructOnScope);
	if (!ensure(DummyRow))
	{
		return;
	}

	DummyRow->CustomWidget(false);
	DummyRow->Visibility(EVisibility::Collapsed);

	ensure(!PropertyHandle);
	PropertyHandle = DummyRow->GetPropertyHandle();

	if (!ensure(PropertyHandle))
	{
		return;
	}

	FVoxelEditorUtilities::TrackHandle(PropertyHandle);

	const FSimpleDelegate PreChangeDelegate = MakeWeakPtrDelegate(this, [this]
	{
		PreEditChange();
	});
	const FSimpleDelegate PostChangeDelegate = MakeWeakPtrDelegate(this, [this]
	{
		PostEditChange();
	});

	PropertyHandle->SetOnPropertyValuePreChange(PreChangeDelegate);
	PropertyHandle->SetOnPropertyValueChanged(PostChangeDelegate);

	PropertyHandle->SetOnChildPropertyValuePreChange(PreChangeDelegate);
	PropertyHandle->SetOnChildPropertyValueChanged(PostChangeDelegate);

	bool bMetadataSet = false;
	TMap<FName, FString> MetaData;
	for (const FVoxelParameterView* ParameterView : ParameterViews)
	{
		TMap<FName, FString> NewMetaData = ParameterView->GetParameter().GetMetaData();
		if (bMetadataSet)
		{
			ensure(MetaData.OrderIndependentCompareEqual(NewMetaData));
		}
		else
		{
			bMetadataSet = true;
			MetaData = MoveTemp(NewMetaData);
		}
	}

	if (!IsOrphan() &&
		// GetInnerType as we also want this to apply to arrays
		RowExposedType.GetInnerType().IsObject())
	{
		const FVoxelPinType PinType = ParameterViews[0]->GetType().GetInnerType();
		const TSharedPtr<const FVoxelObjectPinType> ObjectPinType = FVoxelObjectPinType::StructToPinType().FindRef(PinType.GetStruct());
		if (ensure(ObjectPinType))
		{
			FString& AllowedClasses = MetaData.FindOrAdd("AllowedClasses");
			AllowedClasses.Reset();

			for (const UClass* Class : ObjectPinType->GetAllowedClasses())
			{
				if (!AllowedClasses.IsEmpty())
				{
					AllowedClasses += ",";
				}

				AllowedClasses += Class->GetPathName();
			}
		}
	}

	ensure(!StructWrapper);
	StructWrapper = FVoxelPinValueCustomizationHelper::CreatePinValueCustomization(
		PropertyHandle.ToSharedRef(),
		DetailInterface,
		MakeWeakPtrDelegate(&OverridesDetails, [&OverridesDetails = OverridesDetails]
		{
			OverridesDetails.ForceRefresh();
		}),
		MetaData,
		[&](FDetailWidgetRow& Row, const TSharedRef<SWidget>& ValueWidget)
		{
			BuildRow(Row, ValueWidget);
		},
		// Used to load/save expansion state
		FAddPropertyParams().UniqueId(FName(Guid.ToString())),
		MakeAttributeLambda(MakeWeakPtrLambda(this, [this]
		{
			return
				bForceEnableOverride ||
				IsOrphan() ||
				IsEnabled() == ECheckBoxState::Checked;
		})));
}

void FVoxelParameterDetails::BuildRow(
	FDetailWidgetRow& Row,
	const TSharedRef<SWidget>& ValueWidget)
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelPinType ExposedType;
	if (ParameterViews.Num() > 0)
	{
		ExposedType = ParameterViews[0]->GetType().GetExposedType();
		for (const FVoxelParameterView* ParameterView : ParameterViews)
		{
			ensure(ExposedType == ParameterView->GetType().GetExposedType());
		}
	}
	else
	{
		ExposedType = OrphanExposedType;
	}

	const float Width = FVoxelPinValueCustomizationHelper::GetValueWidgetWidthByType(PropertyHandle, ExposedType);

	const auto GetRowToolTip = MakeWeakPtrLambda(this, [this]() -> FText
	{
		if (ParameterViews.Num() == 0)
		{
			return {};
		}

		const FString Description = ParameterViews[0]->GetDescription();
		for (const FVoxelParameterView* ParameterView : ParameterViews)
		{
			ensure(Description == ParameterView->GetDescription());
		}
		return FText::FromString(Description);
	});

	TSharedRef<SWidget> NameWidget =
		SNew(SVoxelDetailText)
		.ColorAndOpacity(IsOrphan() ? FStyleColors::Error : FSlateColor::UseForeground())
		.Text_Lambda(MakeWeakPtrLambda(this, [this]
		{
			if (DisplayName.IsEmpty())
			{
				return FText::FromString(GetRowName());
			}

			return FText::FromString(DisplayName);
		}))
		.ToolTipText_Lambda(GetRowToolTip);

	if (!bForceEnableOverride &&
		!IsOrphan())
	{
		const TAttribute<bool> EnabledAttribute = MakeAttributeLambda(MakeWeakPtrLambda(this, [this]
		{
			return IsEnabled() == ECheckBoxState::Checked;
		}));

		Row.IsEnabled(EnabledAttribute);
		NameWidget->SetEnabled(EnabledAttribute);
		ValueWidget->SetEnabled(EnabledAttribute);

		NameWidget =
			SNew(SVoxelAlwaysEnabledWidget)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SCheckBox)
					.IsChecked_Lambda(MakeWeakPtrLambda(this, [this]
					{
						return IsEnabled();
					}))
					.OnCheckStateChanged_Lambda(MakeWeakPtrLambda(this, [this](const ECheckBoxState NewState)
					{
						ensure(NewState != ECheckBoxState::Undetermined);
						SetEnabled(NewState == ECheckBoxState::Checked);
					}))
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.FillWidth(1.f)
				[
					NameWidget
				]
			];
	}

	Row
	.FilterString(FText::FromString(GetRowName()))
	.NameContent()
	[
		NameWidget
	]
	.ValueContent()
	.MinDesiredWidth(Width)
	.MaxDesiredWidth(Width)
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SVoxelDetailText)
			.Text(INVTEXT("Multiple Values"))
			.Visibility_Lambda(MakeWeakPtrLambda(this, [this]
			{
				return HasSingleValue() ? EVisibility::Collapsed : EVisibility::Visible;
			}))
		]
		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda(MakeWeakPtrLambda(this, [this]
			{
				return HasSingleValue() ? EVisibility::Visible : EVisibility::Collapsed;
			}))
			[
				ValueWidget
			]
		]
	]
	.Visibility(MakeAttributeLambda(MakeWeakPtrLambda(this, [this]
	{
		return bIsVisible ? EVisibility::Visible : EVisibility::Hidden;
	})))
	.EditCondition(MakeAttributeLambda(MakeWeakPtrLambda(this, [this]
	{
		return !bIsReadOnly;
	})), {})
	.OverrideResetToDefault(FResetToDefaultOverride::Create(
		MakeAttributeLambda(MakeWeakPtrLambda(this, [this]
		{
			return CanResetToDefault();
		})),
		MakeWeakPtrDelegate(this, [this]
		{
			ResetToDefault();
		}),
		false));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ECheckBoxState FVoxelParameterDetails::IsEnabled() const
{
	ensure(!bForceEnableOverride);
	ensure(!IsOrphan());

	bool bAnyEnabled = false;
	bool bAnyDisabled = false;
	for (const FVoxelParameterOverridesDetails::FOwner& Owner : OverridesDetails.GetOwners())
	{
		if (const FVoxelParameterValueOverride* ValueOverride = Owner.Parameters.GetGuidToValueOverride().Find(Guid))
		{
			if (ValueOverride->bEnable)
			{
				bAnyEnabled = true;
			}
			else
			{
				bAnyDisabled = true;
			}
		}
		else
		{
			bAnyDisabled = true;
		}
	}

	if (bAnyEnabled && !bAnyDisabled)
	{
		return ECheckBoxState::Checked;
	}
	if (!bAnyEnabled && bAnyDisabled)
	{
		return ECheckBoxState::Unchecked;
	}
	return ECheckBoxState::Undetermined;
}

void FVoxelParameterDetails::SetEnabled(const bool bNewEnabled) const
{
	ensure(!bForceEnableOverride);
	ensure(!IsOrphan());

	const FScopedTransaction Transaction(FText::FromString((bNewEnabled ? "Enable " : "Disable ") + GetRowName()));

	const TVoxelArray<FVoxelParameterOverridesDetails::FOwner> Owners = OverridesDetails.GetOwners();
	if (!ensure(Owners.Num() == ParameterViews.Num()))
	{
		return;
	}

	for (int32 Index = 0; Index < Owners.Num(); Index++)
	{
		const FVoxelParameterOverridesDetails::FOwner& Owner = Owners[Index];
		Owner.PreEditChange();

		if (FVoxelParameterValueOverride* ExistingValueOverride = Owner.Parameters.GetGuidToValueOverride().Find(Guid))
		{
			ExistingValueOverride->bEnable = bNewEnabled;
			ensure(ExistingValueOverride->Value.IsValid());
		}
		else
		{
			const FVoxelParameterView* ParameterView = ParameterViews[Index];
			if (!ensure(ParameterView))
			{
				continue;
			}

			FVoxelParameterValueOverride ValueOverride;
			ValueOverride.bEnable = true;
			ValueOverride.Value = ParameterView->GetValue();

			// Add AFTER doing GetValue so we don't query ourselves
			Owner.Parameters.GetGuidToValueOverride().Add(Guid, ValueOverride);
		}

		Owner.Parameters.FixupParameterOverrides();
		Owner.PostEditChange();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelParameterDetails::CanResetToDefault() const
{
	if (IsOrphan())
	{
		return true;
	}

	const TVoxelArray<FVoxelParameterOverridesDetails::FOwner> Owners = OverridesDetails.GetOwners();
	if (!ensure(Owners.Num() == ParameterViews.Num()))
	{
		return false;
	}

	for (int32 Index = 0; Index < Owners.Num(); Index++)
	{
		const FVoxelParameterOverridesDetails::FOwner& Owner = Owners[Index];
		const FVoxelParameterView* ParameterView = ParameterViews[Index];
		if (!ensure(ParameterView))
		{
			continue;
		}

		const FVoxelParameterValueOverride* ValueOverride = Owner.Parameters.GetGuidToValueOverride().Find(Guid);
		if (!ValueOverride)
		{
			continue;
		}

		ParameterView->Context.DisableMainOverridesOwner();
		const FVoxelPinValue DefaultValue = ParameterView->GetValue();
		ParameterView->Context.EnableMainOverridesOwner();

		if (ValueOverride->Value != DefaultValue)
		{
			return true;
		}
	}
	return false;
}

void FVoxelParameterDetails::ResetToDefault()
{
	const FScopedTransaction Transaction(FText::FromString("Reset " + GetRowName() + " to default"));

	if (IsOrphan())
	{
		for (const FVoxelParameterOverridesDetails::FOwner& Owner : OverridesDetails.GetOwners())
		{
			Owner.PreEditChange();
			Owner.Parameters.GetGuidToValueOverride().Remove(Guid);
			Owner.Parameters.FixupParameterOverrides();
			Owner.PostEditChange();

			// No need to broadcast OnChanged for orphans
		}

		// Force refresh to remove orphans rows that were removed
		OverridesDetails.ForceRefresh();
		return;
	}

	const TVoxelArray<FVoxelParameterOverridesDetails::FOwner> Owners = OverridesDetails.GetOwners();
	if (!ensure(Owners.Num() == ParameterViews.Num()))
	{
		return;
	}

	for (int32 Index = 0; Index < Owners.Num(); Index++)
	{
		const FVoxelParameterOverridesDetails::FOwner& Owner = Owners[Index];
		const FVoxelParameterView* ParameterView = ParameterViews[Index];
		if (!ensure(ParameterView))
		{
			continue;
		}

		FVoxelParameterValueOverride* ValueOverride = Owner.Parameters.GetGuidToValueOverride().Find(Guid);
		if (!ValueOverride)
		{
			// We might be able to only reset to default one of the multi-selected objects
			ensure(OverridesDetails.GetOwners().Num() > 1);
			continue;
		}

		ParameterView->Context.DisableMainOverridesOwner();
		const FVoxelPinValue DefaultValue = ParameterView->GetValue();
		ParameterView->Context.EnableMainOverridesOwner();

		Owner.PreEditChange();
		ValueOverride->Value = DefaultValue;
		Owner.Parameters.FixupParameterOverrides();
		Owner.PostEditChange();

		// Do this now as caller will broadcast PostChangeDelegate
		SyncFromViews();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelParameterDetails::PreEditChange() const
{
	for (const FVoxelParameterOverridesDetails::FOwner& Owner : OverridesDetails.GetOwners())
	{
		Owner.PreEditChange();
	}
}

void FVoxelParameterDetails::PostEditChange() const
{
	FVoxelInvalidationScope Scope(GetRowName() + " changed");

	for (const FVoxelParameterOverridesDetails::FOwner& Owner : OverridesDetails.GetOwners())
	{
		FVoxelParameterValueOverride& ValueOverride = Owner.Parameters.GetGuidToValueOverride().FindOrAdd(Guid);
		if (bForceEnableOverride)
		{
			ValueOverride.bEnable = true;
		}
		else
		{
			ensure(ValueOverride.bEnable);
		}

		ValueOverride.Value = GetValueRef();
		Owner.Parameters.FixupParameterOverrides();
		Owner.PostEditChange();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelParameterDetails::SyncFromViews()
{
	ensure(!IsOrphan());

	bHasSingleValue = true;

	bool bValueIsSet = false;
	FVoxelPinValue Value;
	for (const FVoxelParameterView* ParameterView : ParameterViews)
	{
		FVoxelPinValue NewValue = ParameterView->GetValue();
		if (bValueIsSet)
		{
			if (Value != NewValue)
			{
				bHasSingleValue = false;
			}
		}
		else
		{
			bValueIsSet = true;
			Value = MoveTemp(NewValue);
		}
	}
	ensure(bValueIsSet);

	// Always set value to the first view, otherwise we can't get Type from property handle
	// Use CopyFrom to not make a copy of the stamp refs, otherwise child details would break
	GetValueRef().CopyFrom(Value);
}

FString FVoxelParameterDetails::GetRowName() const
{
	if (ParameterViews.Num() == 0)
	{
		return OrphanName.ToString();
	}

	const FName Name = ParameterViews[0]->GetName();
	for (const FVoxelParameterView* ParameterView : ParameterViews)
	{
		ensure(Name == ParameterView->GetName());
	}
	return Name.ToString();
}

FVoxelPinValue& FVoxelParameterDetails::GetValueRef() const
{
	FVoxelPinValue* Value = StructOnScope->Get();
	check(Value);
	return *Value;
}