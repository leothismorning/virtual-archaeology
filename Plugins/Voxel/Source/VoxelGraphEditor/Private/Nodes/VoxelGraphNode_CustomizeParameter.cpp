// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphNode_CustomizeParameter.h"
#include "VoxelGraphToolkit.h"
#include "VoxelGraphTracker.h"
#include "VoxelGraphVisuals.h"
#include "Nodes/VoxelNode_CustomizeParameter.h"

const FVoxelParameter* UVoxelGraphNode_CustomizeParameter::GetParameter() const
{
	const UVoxelGraph* Graph = GetTypedOuter<UVoxelGraph>();
	if (!ensure(Graph))
	{
		return nullptr;
	}

	const FVoxelParameter* Parameter = Graph->FindParameter(Guid);
	if (!Parameter)
	{
		return nullptr;
	}

	ConstCast(this)->CachedParameter = *Parameter;
	return Parameter;
}

FVoxelParameter UVoxelGraphNode_CustomizeParameter::GetParameterSafe() const
{
	if (const auto* Parameter = GetParameter())
	{
		return *Parameter;
	}

	return CachedParameter;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphNode_CustomizeParameter::AllocateDefaultPins()
{
	const UVoxelGraph* Graph = GetTypedOuter<UVoxelGraph>();
	if (ensure(Graph))
	{
		OnParameterChangedPtr = MakeSharedVoid();
		GVoxelGraphTracker->OnParameterChanged(*Graph).Add(FOnVoxelGraphChanged::Make(OnParameterChangedPtr, this, [=, this]
		{
			ReconstructNode();
		}));
	}

	const FVoxelParameter Parameter = GetParameterSafe();

	FVoxelNode_CustomizeParameter Template;

	for (const FVoxelPin& Pin : Template.GetPins())
	{
		ensure(Pin.bIsInput);

		UEdGraphPin* GraphPin = CreatePin(
			EGPD_Input,
			Pin.GetType().GetEdGraphPinType(),
			Pin.Name);

		GraphPin->PinFriendlyName = FText::FromString(Pin.Metadata.DisplayName);
		GraphPin->PinToolTip = Pin.Metadata.Tooltip.Get();

		InitializeDefaultValue(Template, Pin, *GraphPin);
	}

	Super::AllocateDefaultPins();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FText UVoxelGraphNode_CustomizeParameter::GetNodeTitle(const ENodeTitleType::Type TitleType) const
{
	if (TitleType != ENodeTitleType::FullTitle)
	{
		return {};
	}

	return FText::FromString("Customize " + GetParameterSafe().Name.ToString());
}

FLinearColor UVoxelGraphNode_CustomizeParameter::GetNodeTitleColor() const
{
	return FLinearColor::Red;
}

FSlateIcon UVoxelGraphNode_CustomizeParameter::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = FVoxelGraphVisuals::GetPinColor(GetParameterSafe().Type);

	return FVoxelGraphVisuals::GetPinIcon(GetParameterSafe().Type);
}

FString UVoxelGraphNode_CustomizeParameter::GetSearchTerms() const
{
	return Guid.ToString();
}