// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "SVoxelGraphPin.h"
#include "Nodes/VoxelGraphNode.h"

TSharedRef<SWidget> SVoxelGraphPin::InitializeEditableLabel(
	const TSharedPtr<SGraphPin>& PinWidget,
	const TSharedRef<SWidget>& DefaultWidget,
	const TAttribute<EVisibility>& PinLabelVisibilityAttribute,
	const TAttribute<FText>& PinLabelAttribute,
	const TAttribute<FSlateColor>& ColorAndOpacityAttribute)
{
	if (!PinWidget->IsEditingEnabled())
	{
		return DefaultWidget;
	}

	const UEdGraphPin* PinObject = PinWidget->GetPinObj();
	if (!ensure(PinObject))
	{
		return DefaultWidget;
	}

	UVoxelGraphNode* OwnerNode = Cast<UVoxelGraphNode>(PinObject->GetOwningNode());
	if (!ensure(OwnerNode))
	{
		return DefaultWidget;
	}

	if (!OwnerNode->CanRenamePin(*PinObject))
	{
		return DefaultWidget;
	}

	TSharedRef<SInlineEditableTextBlock> EditableTextBoxWidget =
		SNew(SInlineEditableTextBlock)
		.Style(&FAppStyle::Get().GetWidgetStyle<FInlineEditableTextBlockStyle>("Graph.Node.InlineEditablePinName"))
		.Visibility(PinLabelVisibilityAttribute)
		.Text(PinLabelAttribute)
		.ColorAndOpacity(ColorAndOpacityAttribute)
		.OnVerifyTextChanged(MakeWeakPtrDelegate(PinWidget, [&PinWidget = *PinWidget](const FText& InNewName, FText& OutErrorMessage)
		{
			UEdGraphPin* EdGraphPin = PinWidget.GetPinObj();
			if (!ensure(EdGraphPin))
			{
				return false;
			}

			const UVoxelGraphNode* Node = Cast<UVoxelGraphNode>(EdGraphPin->GetOwningNode());
			if (!ensure(Node))
			{
				return false;
			}

			return Node->IsNewPinNameValid(*EdGraphPin, FName(InNewName.ToString()));
		}))
		.OnTextCommitted(MakeWeakPtrDelegate(PinWidget, [&PinWidget = *PinWidget](const FText& InNewName, ETextCommit::Type CommitType)
		{
			UEdGraphPin* EdGraphPin = PinWidget.GetPinObj();
			if (!ensure(EdGraphPin))
			{
				return;
			}

			UVoxelGraphNode* Node = Cast<UVoxelGraphNode>(EdGraphPin->GetOwningNode());
			if (!ensure(Node))
			{
				return;
			}

			const FString NewPinName = InNewName.ToString().TrimStartAndEnd();
			if (NewPinName.IsEmpty())
			{
				return;
			}

			Node->RenamePin(*EdGraphPin, FName(NewPinName));
		}));

	OwnerNode->AddPinToRename(PinObject->PinName, MakeWeakPtrDelegate(EditableTextBoxWidget, [&EditableTextBoxWidget = *EditableTextBoxWidget]
	{
		EditableTextBoxWidget.EnterEditingMode();
	}));

	if (OwnerNode->ShouldPromptRenameOnSpawn(*PinObject))
	{
		FVoxelUtilities::DelayedCall(MakeWeakPtrLambda(EditableTextBoxWidget, [&EditableTextBoxWidget = *EditableTextBoxWidget]
		{
			EditableTextBoxWidget.EnterEditingMode();
		}));
	}

	return EditableTextBoxWidget;
}