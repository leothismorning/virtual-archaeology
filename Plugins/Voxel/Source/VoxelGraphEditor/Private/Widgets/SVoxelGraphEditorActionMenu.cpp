// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "SVoxelGraphEditorActionMenu.h"
#include "SGraphActionMenu.h"
#include "VoxelGraphSchemaAction.h"
#include "VoxelGraphContextActionsBuilder.h"
#include "Widgets/Notifications/SProgressBar.h"

void SVoxelGraphActionWidget::Construct(const FArguments& InArgs, const FCreateWidgetForActionData* InCreateData)
{
	ActionPtr = InCreateData->Action;
	MouseButtonDownDelegate = InCreateData->MouseButtonDownDelegate;

	FSlateIcon Icon("EditorStyle", "NoBrush");
	FLinearColor Color = FLinearColor::White;

	if (InCreateData->Action->GetTypeId() == FVoxelGraphSchemaAction::StaticGetTypeId())
	{
		static_cast<FVoxelGraphSchemaAction&>(*InCreateData->Action).GetIcon(Icon, Color);
	}

	this->ChildSlot
	[
		SNew(SHorizontalBox)
		.ToolTipText(InCreateData->Action->GetTooltipDescription())
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SImage)
			.Image(Icon.GetIcon())
			.ColorAndOpacity(Color)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(6.0f, 0.0f, 3.0f, 0.0f)
		[
			SNew(STextBlock)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			.Text(InCreateData->Action->GetMenuDescription())
			.HighlightText(InArgs._HighlightText)
		]
	];
}

FReply SVoxelGraphActionWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseButtonDownDelegate.Execute(ActionPtr))
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelGraphEditorActionMenu::Construct(const FArguments& InArgs)
{
	WeakEdGraph = InArgs._GraphObj;
	DraggedFromPins = InArgs._DraggedFromPins;
	NewNodePosition = InArgs._NewNodePosition;
	OnClosedCallback = InArgs._OnClosedCallback;
	bAutoExpandActionMenu = InArgs._AutoExpandActionMenu;
	OnActionSelected = InArgs._OnActionSelected;
	bNoEdGraph = InArgs._NoEdGraph;

	ensure(bNoEdGraph || InArgs._GraphObj);

	SBorder::Construct(
		SBorder::FArguments()
		.BorderImage(FAppStyle::GetBrush("Menu.Background"))
		.Padding(5.f)
		[
			SNew(SBox)
			.WidthOverride(400.f)
			.HeightOverride(400.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.FillHeight(1.f)
				[
					SAssignNew(GraphActionMenu, SGraphActionMenu)
					.OnActionSelected_Lambda([this](const TArray<TSharedPtr<FEdGraphSchemaAction>>& SelectedActions, const ESelectInfo::Type SelectionType)
					{
						if (SelectionType != ESelectInfo::OnMouseClick &&
							SelectionType != ESelectInfo::OnKeyPress)
						{
							return;
						}

						if (SelectedActions.Num() == 0)
						{
							return;
						}
						ensure(SelectedActions.Num() == 1);

						UEdGraph* EdGraph = WeakEdGraph.Resolve();
						for (const TSharedPtr<FEdGraphSchemaAction>& CurrentAction : SelectedActions)
						{
							if (!CurrentAction.IsValid())
							{
								continue;
							}

							OnActionSelected.ExecuteIfBound(CurrentAction, EdGraph, DraggedFromPins, NewNodePosition);
						}
					})
					.OnGetActionList(this, &SVoxelGraphEditorActionMenu::GetActionList)
					.AutoExpandActionMenu(bAutoExpandActionMenu)
					.DraggedFromPins(DraggedFromPins)
					.GraphObj(WeakEdGraph.Resolve())
					.OnCreateCustomRowExpander(this, &SVoxelGraphEditorActionMenu::CreateActionExpander)
					.OnCreateWidgetForAction(this, &SVoxelGraphEditorActionMenu::OnCreateWidgetForAction)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox)
					.HeightOverride(2.f)
					.Visibility_Lambda([this]
					{
						return
							ActionsBuilder &&
							!ActionsBuilder->IsCompleted()
							? EVisibility::Visible
							: EVisibility::Collapsed;
					})
					[
						SNew(SProgressBar)
						.BorderPadding(FVector2D::ZeroVector)
						.Percent_Lambda([this]()
						{
							return ActionsBuilder ? ActionsBuilder->GetProgress() : 0.0f;
						})
					]
				]
			]
		]
	);
}

void SVoxelGraphEditorActionMenu::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SBorder::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (ActionsBuilder)
	{
		const int32 NewIdxStart = RootActionList->GetNumActions();
		if (ActionsBuilder->Build())
		{
			GraphActionMenu->UpdateForNewActions(NewIdxStart);
		}
	}
}

SVoxelGraphEditorActionMenu::~SVoxelGraphEditorActionMenu()
{
	OnClosedCallback.ExecuteIfBound();
}

TSharedRef<SEditableTextBox> SVoxelGraphEditorActionMenu::GetFilterTextBox() const
{
	return GraphActionMenu->GetFilterTextBox();
}

TSharedRef<FGraphActionListBuilderBase> SVoxelGraphEditorActionMenu::GetActionList()
{
	RootActionList = MakeShared<FGraphContextMenuBuilder>(WeakEdGraph.Resolve());
	if (DraggedFromPins.Num() != 0)
	{
		RootActionList->FromPin = DraggedFromPins[0];
	}

	if (bNoEdGraph)
	{
		ActionsBuilder = FVoxelGraphContextActionsBuilder::BuildNoGraph(RootActionList);
	}
	else
	{
		ActionsBuilder = FVoxelGraphContextActionsBuilder::Build(RootActionList);
	}

	return RootActionList.ToSharedRef();
}

TSharedRef<SExpanderArrow> SVoxelGraphEditorActionMenu::CreateActionExpander(const FCustomExpanderData& ActionMenuData) const
{
	return SNew(SExpanderArrow, ActionMenuData.TableRow);
}

TSharedRef<SWidget> SVoxelGraphEditorActionMenu::OnCreateWidgetForAction(FCreateWidgetForActionData* CreateData) const
{
	return SNew(SVoxelGraphActionWidget, CreateData);
}