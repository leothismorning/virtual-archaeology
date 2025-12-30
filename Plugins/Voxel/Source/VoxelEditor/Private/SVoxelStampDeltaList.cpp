// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "SVoxelStampDeltaList.h"
#include "VoxelLayerStack.h"
#include "VoxelHeightStamp.h"
#include "Styling/SlateIconFinder.h"

VOXEL_INITIALIZE_STYLE(VoxelStampDeltaListStyle)

{
	const FButtonStyle Button = FAppStyle::GetWidgetStyle<FButtonStyle>("Button");
	const FSlateColor SelectionColor = FAppStyle::GetSlateColor("SelectionColor");
	const FSlateColor SelectionColor_Inactive = FAppStyle::GetSlateColor("SelectionColor_Inactive");
	const FSlateColor SelectionColor_Pressed = FAppStyle::GetSlateColor("SelectionColor_Pressed");

	Set("Voxel.OpenInExternalEditor", FButtonStyle(Button)
		.SetNormal(CORE_IMAGE_BRUSH_SVG("Starship/Common/OpenInExternalEditor", CoreStyleConstants::Icon16x16))
		.SetHovered(CORE_IMAGE_BRUSH_SVG("Starship/Common/OpenInExternalEditor", CoreStyleConstants::Icon16x16, SelectionColor))
		.SetPressed(CORE_IMAGE_BRUSH_SVG("Starship/Common/OpenInExternalEditor", CoreStyleConstants::Icon16x16, SelectionColor_Pressed))
		.SetDisabled(CORE_IMAGE_BRUSH_SVG("Starship/Common/OpenInExternalEditor", CoreStyleConstants::Icon16x16, SelectionColor_Inactive)));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelStampDeltaListEntry::Construct(
	const FArguments& Args,
	const TSharedRef<STableViewBase>& OwnerTableView,
	const FVoxelStampDelta& NewStampDelta)
{
	StampDelta = NewStampDelta;

	FSuperRowType::Construct(FSuperRowType::FArguments(), OwnerTableView);
}

TSharedRef<SWidget> SVoxelStampDeltaListEntry::GenerateWidgetForColumn(const FName& ColumnName)
{
	const FVoxelStampRuntime& Stamp = *StampDelta.Stamp;

	if (ColumnName == "StampName")
	{
		const AActor* Actor = Stamp.GetActor();
		const int32 InstanceIndex = Stamp.GetInstanceIndex();

		return
			SNew(SBox)
			.MinDesiredHeight(24.f)
			.Padding(4.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4.f, 0.f)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(FSlateIconFinder::FindIconBrushForClass(Actor ? Actor->GetClass() : nullptr))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 1.f, 0.f, 0.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
							.Text(FText::FromString(Stamp.GetComponent().GetReadableName()))
							.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
							.ColorAndOpacity(FSlateColor::UseForeground())
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
							.Text(FText::FromString(" [" + FText::AsNumber(InstanceIndex).ToString() + "]"))
							.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
							.ColorAndOpacity(FSlateColor::UseSubduedForeground())
							.Visibility(InstanceIndex == -1 ? EVisibility::Collapsed : EVisibility::Visible)
						]
					]
				]
			];
	}

	if (ColumnName == "BlendMode")
	{
		return
			SNew(SBox)
			.Padding(4.f, 0.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Stamp.GetStamp().GetActorLabelInfo().BlendMode))
				.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				.ColorAndOpacity(FSlateColor::UseForeground())
			];
	}

	if (ColumnName == "Layer")
	{
		return
			SNew(SBox)
			.Padding(4.f, 0.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(StampDelta.Layer.Layer.GetReadableName()))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			];
	}

	if (ColumnName == "Type")
	{
		return
			SNew(SBox)
			.Padding(4.f, 0.f)
			[
				SNew(STextBlock)
				.Text(Stamp.IsA<FVoxelHeightStampRuntime>() ? INVTEXT("Height") : INVTEXT("Volume"))
				.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				.ColorAndOpacity(FSlateColor::UseForeground())
			];
	}

	if (ColumnName == "Priority")
	{
		return
			SNew(SBox)
			.Padding(4.f, 0.f)
			[
				SNew(STextBlock)
				.Text(FText::AsNumber(Stamp.GetStamp().Priority))
				.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				.ColorAndOpacity(FSlateColor::UseForeground())
			];
	}

	if (ColumnName == "Value")
	{
		return
			SNew(SBox)
			.Padding(4.f, 0.f)
			[
				SNew(STextBlock)
				.Text(FText::AsNumber(StampDelta.DistanceAfter))
				.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				.ColorAndOpacity(FSlateColor::UseForeground())
			];
	}

	return SNullWidget::NullWidget;
}

void SVoxelStampDeltaListEntry::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FSuperRowType::OnMouseEnter(MyGeometry, MouseEvent);

	if (StampDelta.Stamp)
	{
		StampDelta.Stamp->SelectComponent_EditorOnly();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelStampDeltaList::Construct(const FArguments& Args)
{
	for (const FVoxelStampDelta& StampDelta : Args._StampDeltas)
	{
		StampDeltaArray.Add(MakeSharedCopy(StampDelta));
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SAssignNew(ListView, SListView<TSharedPtr<FVoxelStampDelta>>)
				.ListViewStyle(&FAppStyle::Get().GetWidgetStyle<FTableViewStyle>("PropertyTable.InViewport.ListView"))
				.SelectionMode(ESelectionMode::Single)
				.ListItemsSource(&StampDeltaArray)
				.HeaderRow(
					SNew(SHeaderRow)
					+ SHeaderRow::Column("StampName")
					.HAlignCell(HAlign_Left)
					.VAlignCell(VAlign_Center)
					.VAlignHeader(VAlign_Center)
					.FillWidth(1.f)
					[
						SNew(SBox)
						.MinDesiredHeight(24.f)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SVoxelDetailText)
							.Text(INVTEXT("Stamp Name"))
						]
					]
					+ SHeaderRow::Column("BlendMode")
					.HAlignCell(HAlign_Center)
					.VAlignCell(VAlign_Center)
					.VAlignHeader(VAlign_Center)
					.FixedWidth(60.f)
					[
						SNew(SBox)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SVoxelDetailText)
							.Text(INVTEXT("Blend Mode"))
						]
					]
					+ SHeaderRow::Column("Value")
					.HAlignCell(HAlign_Center)
					.VAlignCell(VAlign_Center)
					.VAlignHeader(VAlign_Center)
					.FixedWidth(60.f)
					[
						SNew(SBox)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SVoxelDetailText)
							.Text(INVTEXT("Value"))
						]
					]
					+ SHeaderRow::Column("Type")
					.HAlignCell(HAlign_Center)
					.VAlignCell(VAlign_Center)
					.VAlignHeader(VAlign_Center)
					.FixedWidth(45.f)
					[
						SNew(SBox)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SVoxelDetailText)
							.Text(INVTEXT("Type"))
						]
					]
					+ SHeaderRow::Column("Priority")
					.HAlignCell(HAlign_Center)
					.VAlignCell(VAlign_Center)
					.VAlignHeader(VAlign_Center)
					.FixedWidth(45.f)
					[
						SNew(SBox)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SVoxelDetailText)
							.Text(INVTEXT("Priority"))
						]
					]
					+ SHeaderRow::Column("Layer")
					.HAlignCell(HAlign_Center)
					.VAlignCell(VAlign_Center)
					.VAlignHeader(VAlign_Center)
					.FixedWidth(150.f)
					[
						SNew(SBox)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SVoxelDetailText)
							.Text(INVTEXT("Layer"))
						]
					]
				)
				.OnGenerateRow_Lambda([this](const TSharedPtr<FVoxelStampDelta>& StampDelta, const TSharedRef<STableViewBase>& OwnerTable)
				{
					return SNew(SVoxelStampDeltaListEntry, OwnerTable, *StampDelta);
				})
				.OnSelectionChanged_Lambda([this](const TSharedPtr<FVoxelStampDelta> StampDelta, ESelectInfo::Type SelectInfo)
				{
					if (!StampDelta)
					{
						return;
					}

					FSlateApplication::Get().DismissAllMenus();

					const FScopedTransaction Transaction(INVTEXT("Clicking on Components"));
					StampDelta->Stamp->SelectComponent_EditorOnly();
				})
			]
		]
	];
}