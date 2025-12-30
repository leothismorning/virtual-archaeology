// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "SVoxelGraphPinHeightExportChannelData.h"
#include "VoxelPinValue.h"
#include "VoxelFloatMetadata.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

void SVoxelGraphPinHeightExportChannelData::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

TSharedRef<SWidget>	SVoxelGraphPinHeightExportChannelData::GetDefaultValueWidget()
{
	const UEnum* Enum = StaticEnumFast<EVoxelHeightExportChannelType>();

	ChannelTypes.Reset();
	ChannelTypes.Add(MakeShared<EVoxelHeightExportChannelType>(EVoxelHeightExportChannelType::None));
	ChannelTypes.Add(MakeShared<EVoxelHeightExportChannelType>(EVoxelHeightExportChannelType::Height));
	ChannelTypes.Add(MakeShared<EVoxelHeightExportChannelType>(EVoxelHeightExportChannelType::Metadata));

	const FVoxelPinValue DefaultValue = FVoxelPinValue::MakeFromPinDefaultValue(*GraphPinObj);
	ChannelData = ensure(DefaultValue.Is<FVoxelHeightExportChannelData>())
		? DefaultValue.Get<FVoxelHeightExportChannelData>()
		: FVoxelHeightExportChannelData();

	return
		SNew(SVerticalBox)
		.Visibility(this, &SGraphPin::GetDefaultValueVisibility)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(ComboButton, SComboButton)
			.ComboButtonStyle(FAppStyle::Get(), "ComboButton")
			.ContentPadding(FMargin(0.f, 2.f))
			.ForegroundColor(FSlateColor::UseForeground())
			.ButtonContent()
			[
				SNew(SBox)
				.MinDesiredWidth(75.f)
				[
					SNew(STextBlock)
					.ToolTipText_Lambda(MakeWeakPtrLambda(this, [this, Enum]
					{
						return Enum->GetToolTipTextByIndex(Enum->GetIndexByValue(uint8(ChannelData.ChannelType)));
					}))
					.Text_Lambda(MakeWeakPtrLambda(this, [this, Enum]
					{
						return Enum->GetDisplayNameTextByValue(uint8(ChannelData.ChannelType));
					}))
					.Font(FAppStyle::GetFontStyle( TEXT("PropertyWindow.NormalFont")))
				]
			]
			.MenuContent()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.MaxHeight(450.f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("Menu.Background"))
					.Padding(0.f)
					[
						SNew(SComboList)
						.ListItemsSource(&ChannelTypes)
						.OnGenerateRow_Lambda([this, Enum](const TSharedPtr<EVoxelHeightExportChannelType> ChannelType, const TSharedRef<STableViewBase>& OwnerTable)
						{
							return
								SNew(STableRow<TSharedPtr<EVoxelHeightExportChannelType>>, OwnerTable)
								[
									SNew(SBox)
									.MinDesiredWidth(150.0f)
									[
										SNew(STextBlock)
										.Text(Enum->GetDisplayNameTextByValue(uint8(*ChannelType)))
										.ToolTipText(Enum->GetToolTipTextByIndex(Enum->GetIndexByValue(uint8(*ChannelType))))
										.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
									]
								];
						})
						.OnSelectionChanged_Lambda(MakeWeakPtrLambda(this, [this](const TSharedPtr<EVoxelHeightExportChannelType> NewSelection, ESelectInfo::Type SelectInfo)
						{
							if (!ensure(!GraphPinObj->IsPendingKill()))
							{
								return;
							}

							const FVoxelTransaction Transaction(GraphPinObj, "Set Channel Type");

							ChannelData.ChannelType = *NewSelection;

							GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, FVoxelPinValue::Make(ChannelData).ExportToString());

							ComboButton->SetIsOpen(false);
						}))
					]
				]
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 4.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			.Visibility_Lambda(MakeWeakPtrLambda(this, [this]
			{
				return ChannelData.ChannelType == EVoxelHeightExportChannelType::Metadata ? EVisibility::Visible : EVisibility::Collapsed;
			}))
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.f,0.f)
			.MaxWidth(100.f)
			[
				SAssignNew(AssetPickerAnchor, SComboButton)
				.ButtonStyle(FAppStyle::Get(), "PropertyEditor.AssetComboStyle")
				.ForegroundColor_Lambda(MakeWeakPtrLambda(this, [this]
				{
					const float Alpha =
						IsHovered() ||
						bOnlyShowDefaultValue
						? 1.f
						: 0.6f;
					return FLinearColor(1.f, 1.f, 1.f, Alpha);
				}))
				.ContentPadding(FMargin(2.f ,2.f ,2.f ,1.f))
				.ButtonColorAndOpacity_Lambda(MakeWeakPtrLambda(this, [this]
				{
					const float Alpha =
						IsHovered() ||
						bOnlyShowDefaultValue
						? 0.8f
						: 0.4f;
					return FLinearColor(1.f, 1.f, 1.f, Alpha);
				}))
				.MenuPlacement(MenuPlacement_BelowAnchor)
				.IsEnabled(this, &SGraphPin::IsEditingEnabled)
				.ButtonContent()
				[
					SNew(STextBlock)
					.ColorAndOpacity_Lambda(MakeWeakPtrLambda(this, [this]
					{
						const float Alpha =
							IsHovered() ||
							bOnlyShowDefaultValue
							? 1.f
							: 0.6f;
						return FLinearColor(1.f, 1.f, 1.f, Alpha);
					}))
					.TextStyle(FAppStyle::Get(), "PropertyEditor.AssetClass")
					.Font(FAppStyle::GetFontStyle( "PropertyWindow.NormalFont"))
					.Text_Lambda(MakeWeakPtrLambda(this, [this]
					{
						if (!ChannelData.Metadata)
						{
							return INVTEXT("Select Asset");
						}
						return FText::FromString(ChannelData.Metadata->GetName());
					}))
				]
				.OnGetMenuContent_Lambda(MakeWeakPtrLambda(this, [this]() -> TSharedRef<SWidget>
				{
					FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

					FAssetPickerConfig AssetPickerConfig;
					AssetPickerConfig.OnAssetSelected = MakeWeakPtrDelegate(this, [this](const FAssetData& Asset)
					{
						ON_SCOPE_EXIT
						{
							AssetPickerAnchor->SetIsOpen(false);
						};

						if (GraphPinObj->IsPendingKill())
						{
							return;
						}

						UVoxelFloatMetadata* NewMetaData = Cast<UVoxelFloatMetadata>(Asset.GetAsset());
						if (ChannelData.Metadata == NewMetaData)
						{
							return;
						}

						const FVoxelTransaction Transaction(GraphPinObj, "Set MetaData");
						ChannelData.Metadata = NewMetaData;

						GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, FVoxelPinValue::Make(ChannelData).ExportToString());
					});
					AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
					AssetPickerConfig.Filter.ClassPaths.Add(UVoxelFloatMetadata::StaticClass()->GetClassPathName());
					return
						SNew(SBox)
						.HeightOverride(300)
						.WidthOverride(300)
						[
							SNew(SBorder)
							.BorderImage( FAppStyle::GetBrush("Menu.Background") )
							[
								ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
							]
						];
				},
				SNullWidget::NullWidget))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(1.f, 0.f)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ButtonColorAndOpacity_Lambda(MakeWeakPtrLambda(this, [this]
				{
					const float Alpha =
						IsHovered() ||
						bOnlyShowDefaultValue
						? 0.8f
						: 0.4f;
					return FLinearColor(1.f, 1.f, 1.f, Alpha);
				}))
				.OnClicked_Lambda(MakeWeakPtrLambda(this, [this]() -> FReply
				{
					if (!ensure(!GraphPinObj->IsPendingKill()))
					{
						return FReply::Handled();
					}

					UVoxelFloatMetadata* SelectedObject = GEditor->GetSelectedObjects()->GetTop<UVoxelFloatMetadata>();
					if (!SelectedObject ||
						ChannelData.Metadata == SelectedObject)
					{
						return FReply::Handled();
					}

					const FVoxelTransaction Transaction(GraphPinObj, "Set MetaData");
					ChannelData.Metadata = SelectedObject;

					GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, FVoxelPinValue::Make(ChannelData).ExportToString());

					return FReply::Handled();
				},
				FReply::Handled()))
				.ContentPadding(1.f)
				.ToolTipText(NSLOCTEXT("GraphEditor", "ObjectGraphPin_Use_Tooltip", "Use asset browser selection"))
				.IsEnabled(this, &SGraphPin::IsEditingEnabled)
				[
					SNew(SImage)
					.ColorAndOpacity_Lambda(MakeWeakPtrLambda(this, [this]
					{
						const float Alpha =
							IsHovered() ||
							bOnlyShowDefaultValue
							? 1.f
							: 0.15f;
						return FLinearColor(1.f, 1.f, 1.f, Alpha);
					}))
					.Image(FAppStyle::GetBrush(TEXT("Icons.CircleArrowLeft")))
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(1,0)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle( FAppStyle::Get(), "NoBorder" )
				.ButtonColorAndOpacity_Lambda(MakeWeakPtrLambda(this, [this]
				{
					const float Alpha =
						IsHovered() ||
						bOnlyShowDefaultValue
						? 0.8f
						: 0.4f;
					return FLinearColor(1.f, 1.f, 1.f, Alpha);
				}))
				.OnClicked_Lambda(MakeWeakPtrLambda(this, [this]
				{
					if (!ChannelData.Metadata)
					{
						return FReply::Handled();
					}

					GEditor->SyncBrowserToObjects({ ChannelData.Metadata });
					return FReply::Handled();
				},
				FReply::Handled()))
				.ContentPadding(0)
				.ToolTipText(NSLOCTEXT("GraphEditor", "ObjectGraphPin_Browse_Tooltip", "Browse"))
				[
					SNew(SImage)
					.ColorAndOpacity_Lambda(MakeWeakPtrLambda(this, [this]
					{
						const float Alpha =
							IsHovered() ||
							bOnlyShowDefaultValue
							? 1.f
							: 0.15f;
						return FLinearColor(1.f, 1.f, 1.f, Alpha);
					}))
					.Image(FAppStyle::GetBrush(TEXT("Icons.Search")))
				]
			]
		];
}