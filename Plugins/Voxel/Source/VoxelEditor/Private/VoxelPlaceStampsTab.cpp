// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelPlaceStampsTab.h"

#include "VoxelGraph.h"
#include "Shape/VoxelShape.h"
#include "VoxelActorFactories.h"
#include "VoxelEditorSettings.h"
#include "VoxelPlaceStampsTile.h"
#include "Heightmap/VoxelHeightmap.h"
#include "Sculpt/Height/VoxelHeightSculptStamp.h"
#include "Sculpt/Volume/VoxelVolumeSculptStamp.h"
#include "StaticMesh/VoxelStaticMesh.h"

#include "LevelEditor.h"
#include "AssetSelection.h"
#include "WidgetDrawerConfig.h"
#include "IPlacementModeModule.h"
#include "ContentBrowserModule.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Toolkits/AssetEditorModeUILayer.h"

class FVoxelPlaceStampsCommands : public TVoxelCommands<FVoxelPlaceStampsCommands>
{
public:
	TSharedPtr<FUICommandInfo> TogglePlaceStampsDrawer;

	virtual void RegisterCommands() override;
};

DEFINE_VOXEL_COMMANDS(FVoxelPlaceStampsCommands);

void FVoxelPlaceStampsCommands::RegisterCommands()
{
#if PLATFORM_MAC
	VOXEL_UI_COMMAND(TogglePlaceStampsDrawer, "Open Place Stamps Drawer", "Opens a temporary window with available voxel stamps to place into world (Shift+Space)", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift, EKeys::SpaceBar));
#else
	VOXEL_UI_COMMAND(TogglePlaceStampsDrawer, "Open Place Stamps Drawer", "Opens a temporary window with available voxel stamps to place into world (Shift+Space)", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift, EKeys::SpaceBar));
#endif
}

VOXEL_RUN_ON_STARTUP_EDITOR()
{
	FLevelEditorModule& Module = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	Module.OnTabManagerChanged().AddLambda([]
	{
		const FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		const TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager();

		LevelEditorTabManager->RegisterTabSpawner("PlaceVoxelStamps", FOnSpawnTab::CreateLambda([](const FSpawnTabArgs& Args)
		{
			TSharedRef<SDockTab> DockTab =
				SNew(SDockTab)
				.Label(INVTEXT("Place Voxel Stamps"));

			DockTab->SetContent(SNew(SVoxelPlaceStampsTab, DockTab));

			return DockTab;
		}))
		.SetDisplayName(INVTEXT("Place Voxel Stamps"))
		.SetTooltipText({})
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.PlacementBrowser"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory());
	});

	Module.OnRegisterLayoutExtensions().AddLambda([](FLayoutExtender& Extender)
	{
		Extender.ExtendLayout(
			LevelEditorTabIds::PlacementBrowser,
			ELayoutExtensionPosition::After,
			FTabManager::FTab(FTabId("PlaceVoxelStamps"), ETabState::ClosedTab));
	});

	FVoxelUtilities::DelayedCall([]
	{
		if (GetDefault<UVoxelEditorSettings>()->bEnablePlaceStampsDrawer)
		{
			SVoxelPlaceStampsTab::RegisterDrawer();
		}
	});

	const auto TogglePlaceStampsDrawer = []
	{
		if (UStatusBarSubsystem* StatusBarSubsystem = GEditor->GetEditorSubsystem<UStatusBarSubsystem>())
		{
			StatusBarSubsystem->TryToggleDrawer("PlaceVoxelStamps");
		}
	};

	const FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetGlobalLevelEditorActions()->MapAction(
		FVoxelPlaceStampsCommands::Get().TogglePlaceStampsDrawer,
		MakeLambdaDelegate(TogglePlaceStampsDrawer));

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.GetAllContentBrowserCommandExtenders().Add(MakeLambdaDelegate([TogglePlaceStampsDrawer](TSharedRef<FUICommandList> OutCommandList, FOnContentBrowserGetSelection InGetSelectionDelegate)
	{
		OutCommandList->MapAction(
			FVoxelPlaceStampsCommands::Get().TogglePlaceStampsDrawer,
			MakeLambdaDelegate(TogglePlaceStampsDrawer));
	}));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class SVoxelPlaceStampsTagCheckBox : public SCheckBox
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ARGUMENT(FName, ContentTag)
		SLATE_EVENT(TDelegate<void(bool)>, OnContentTagClicked)
		SLATE_EVENT(FSimpleDelegate, OnContentTagShiftClicked)
		SLATE_ATTRIBUTE(ECheckBoxState, IsChecked)
	};

	void Construct(const FArguments& Args)
	{
		OnContentTagClicked = Args._OnContentTagClicked;
		OnContentTagShiftClicked = Args._OnContentTagShiftClicked;

		const FName ContentTag = Args._ContentTag;
		const FLinearColor TagColor = SVoxelPlaceStampsTab::GetStampTagColor(ContentTag);

		SCheckBox::Construct(
			SCheckBox::FArguments()
			.Style(FAppStyle::Get(), "ContentBrowser.FilterButton")
			.IsChecked(Args._IsChecked)
			.OnCheckStateChanged_Lambda([this](const ECheckBoxState NewState)
			{
				OnContentTagClicked.ExecuteIfBound(NewState == ECheckBoxState::Checked);
			}));

		SetToolTipText(FText::FromString("Display content with tag: " + FName::NameToDisplayString(ContentTag.ToString(), false) + """.\nUse Shift+Click to exclusively select this tag."));

		SetContent(
			SNew(SBorder)
			.Padding(1.f)
			.BorderImage(FVoxelEditorStyle::GetBrush("Graph.NewAssetDialog.FilterCheckBox.Border"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("ContentBrowser.FilterImage"))
					.ColorAndOpacity_Lambda([=, this]
					{
						FLinearColor Color = TagColor;
						if (!IsChecked())
						{
							Color.A = 0.1f;
						}

						return Color;
					})
				]
				+ SHorizontalBox::Slot()
				.Padding(MakeAttributeLambda([this]
				{
					return bIsPressed ? FMargin(4.f, 2.f, 3.f, 0.f) : FMargin(4.f, 1.f, 3.f, 1.f);
				}))
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(FText::FromString(FName::NameToDisplayString(ContentTag.ToString(), false)))
					.ColorAndOpacity_Lambda([this]
					{
						if (IsHovered())
						{
							return FLinearColor(0.75f, 0.75f, 0.75f, 1.f);
						}

						return IsChecked() ? FLinearColor::White : FLinearColor::Gray;
					})
					.TextStyle(FVoxelEditorStyle::Get(), "Graph.NewAssetDialog.ActionFilterTextBlock")
				]
			]
		);
	}

	//~ Begin SCheckBox Interface
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		const FReply Reply = SCheckBox::OnMouseButtonUp(MyGeometry, MouseEvent);

		if (FSlateApplication::Get().GetModifierKeys().IsShiftDown() &&
			MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			OnContentTagShiftClicked.ExecuteIfBound();
			return FReply::Handled().ReleaseMouseCapture();
		}

		return Reply;
	}
	//~ End SCheckBox Interface

private:
	TDelegate<void(bool)> OnContentTagClicked;
	FSimpleDelegate OnContentTagShiftClicked;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelPlaceStampsTab::Construct(const FArguments& InArgs, const TSharedPtr<SDockTab>& ParentTab)
{
	FillFavoritesList();

	Filter = MakeShared<FContentSourceTextFilter>(
		FContentSourceTextFilter::FItemToStringArray::CreateLambda([](const TSharedPtr<FVoxelPlaceStampItem> Item, TArray<FString>& Array)
		{
			Array.Add(Item->Name);
		})
	);

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();
	AssetRegistry.OnAssetsAdded().AddSP(this, &SVoxelPlaceStampsTab::OnAssetsAdded);
	AssetRegistry.OnAssetRenamed().AddSP(this, &SVoxelPlaceStampsTab::OnAssetRenamed);
	AssetRegistry.OnAssetUpdated().AddSP(this, &SVoxelPlaceStampsTab::OnAssetUpdated);
	AssetRegistry.OnAssetRemoved().AddSP(this, &SVoxelPlaceStampsTab::OnAssetRemoved);

	if (ParentTab)
	{
		ParentTab->SetOnTabDrawerOpened(MakeWeakPtrDelegate(this, [this]
		{
			FSlateApplication::Get().SetKeyboardFocus(SearchBox, EFocusCause::SetDirectly);
		}));
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
		.Padding(0.f)
		[
			SNew(SSplitter)
			.PhysicalSplitterHandleSize(2.f)
			+ SSplitter::Slot()
			.Resizable(false)
			.SizeRule(SSplitter::SizeToContent)
			[
				CreateCategoriesWidget()
			]
			+ SSplitter::Slot()
			.SizeRule(SSplitter::FractionOfParent)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("Brushes.Recessed"))
				[
					SNew( SVerticalBox )
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
						.Padding(8.f)
						[
							SAssignNew(SearchBox, SSearchBox)
							.HintText(INVTEXT("Search Stamps"))
							.OnTextChanged_Lambda([this](const FText& Text)
							{
								Filter->SetRawFilterText(Text);
								UpdateFilteredItems();
							})
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 8.f)
					[
						SAssignNew(TagsBox, SWrapBox)
						.UseAllottedSize(true)
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.f)
					[
						SAssignNew(WidgetSwitcher, SWidgetSwitcher)
						+ SWidgetSwitcher::Slot()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SAssignNew(MessageTextBlock, STextBlock)
							.ColorAndOpacity(FSlateColor::UseSubduedForeground())
							.Text({})
						]
						+ SWidgetSwitcher::Slot()
						[
							SAssignNew(TileView, STileView<TSharedPtr<FVoxelPlaceStampItem>>)
							.ListItemsSource(&FilteredCategoryItems)
							.OnGenerateTile_Lambda([this](const TSharedPtr<FVoxelPlaceStampItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
							{
								return
									SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
									.Style(FAppStyle::Get(), "ProjectBrowser.TableRow")
									.Padding(2.f)
									[
										SNew(SVoxelPlaceStampsTile, Item)
										.VisibleTags_Lambda([this]
										{
											return VisibleTags;
										})
										.OnFavoriteStateChanged_Lambda([this, Item](const bool bNewFavorite)
										{
											const FSoftObjectPath ObjectPath = Item->Asset.GetSoftObjectPath();
											if (bNewFavorite)
											{
												Favorites.Add(ObjectPath);
											}
											else
											{
												Favorites.Remove(ObjectPath);
												if (ActiveTabName == "Favorites")
												{
													AllCategoryItems.Remove(Item);
													TrackedAssetToItem.Remove(ObjectPath);
													UpdateTags(false);
													UpdateFilteredItems();
												}
											}

											UpdateFavorites();
										})
									];
							})
							.ClearSelectionOnClick(false)
							.ItemAlignment(EListItemAlignment::EvenlySize)
							.ItemWidth(122.f)
							.ItemHeight(197.f)
							.SelectionMode(ESelectionMode::None)
						]
					]
				]
			]
		]
	];

	SetActiveTab("Shapes");
}

void SVoxelPlaceStampsTab::FocusSearchBox() const
{
	FSlateApplication::Get().SetKeyboardFocus(SearchBox, EFocusCause::SetDirectly);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> SVoxelPlaceStampsTab::CreateCategoriesWidget()
{
	FVerticalToolBarBuilder ToolbarBuilder(
		nullptr,
		FMultiBoxCustomization::None,
		TSharedPtr<FExtender>(),
		true);
	ToolbarBuilder.SetLabelVisibility(EVisibility::Visible);
	ToolbarBuilder.SetStyle(&FAppStyle::Get(), "CategoryDrivenContentBuilderToolbarWithLabels");

	const auto AddCategory = [&](const FName Name, const FSlateIcon& Icon, const FCollectItemsFunction& Function)
	{
		CategoryToCollectFunction.Add(Name, Function);
		ToolbarBuilder.AddToolBarButton(
			FUIAction(
				MakeWeakPtrDelegate(this, [this, Name]
				{
					SetActiveTab(Name);
				}),
				{},
				MakeWeakPtrDelegate(this, [this, Name]
				{
					return ActiveTabName == Name;
				})),
			{},
			FText::FromName(Name),
			{},
			Icon,
			EUserInterfaceActionType::ToggleButton);
	};

	AddCategory(
		"Favorites",
		FSlateIcon(FAppStyle::Get().GetStyleSetName(), "Icons.Favorites.Small"),
		MakeLambdaDelegate([this](TArray<TSharedPtr<FVoxelPlaceStampItem>>& OutItems)
		{
			CollectAllFavorites(OutItems);
		}));
	AddCategory(
		"Recent",
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "PlacementBrowser.Icons.Recent"),
		MakeLambdaDelegate([](TArray<TSharedPtr<FVoxelPlaceStampItem>>& OutItems)
		{
			CollectAllRecent(OutItems);
		}));
	AddCategory(
		"Shapes",
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Cube"),
		MakeLambdaDelegate([](TArray<TSharedPtr<FVoxelPlaceStampItem>>& OutItems)
		{
			for (UScriptStruct* Struct : GetDerivedStructs<FVoxelShape>())
			{
				OutItems.Add(ConstructShape(FAssetData(Struct)));
			}
		}));
	AddCategory(
		"Graphs",
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Blueprint"),
		MakeLambdaDelegate([](TArray<TSharedPtr<FVoxelPlaceStampItem>>& OutItems)
		{
			ForEachAssetDataOfClass<UVoxelGraph>([&](const FAssetData& Asset)
			{
				OutItems.Add(ConstructGraph(Asset));
			});
		}));
	AddCategory(
		"Meshes",
		FSlateIcon(FVoxelEditorStyle::GetStyleSetName(), "ClassIcon.VoxelStaticMesh"),
		MakeLambdaDelegate([](TArray<TSharedPtr<FVoxelPlaceStampItem>>& OutItems)
		{
			ForEachAssetDataOfClass<UVoxelStaticMesh>([&](const FAssetData& Asset)
			{
				OutItems.Add(ConstructMesh(Asset));
			});
		}));
	AddCategory(
		"Heightmaps",
		FSlateIcon(FVoxelEditorStyle::GetStyleSetName(), "ClassIcon.VoxelGraphHeightmap"),
		MakeLambdaDelegate([](TArray<TSharedPtr<FVoxelPlaceStampItem>>& OutItems)
		{
			ForEachAssetDataOfClass<UVoxelHeightmap>([&](const FAssetData& Asset)
			{
				OutItems.Add(ConstructHeightmap(Asset));
			});
		}));
	AddCategory(
		"Sculpt",
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LandscapeEditor.SculptTool"),
		MakeLambdaDelegate([this](TArray<TSharedPtr<FVoxelPlaceStampItem>>& OutItems)
		{
			OutItems.Add(ConstructSculpt(FAssetData(FVoxelHeightSculptStamp::StaticStruct())));
			OutItems.Add(ConstructSculpt(FAssetData(FVoxelVolumeSculptStamp::StaticStruct())));
		}));

	return ToolbarBuilder.MakeWidget();
}

void SVoxelPlaceStampsTab::SetActiveTab(const FName Name)
{
	ActiveTabName = Name;
	UpdateContentForCategory(Name);
}

void SVoxelPlaceStampsTab::UpdateContentForCategory(const FName CategoryName)
{
	TrackedAssetToItem = {};
	AllCategoryItems = {};

	if (const FCollectItemsFunction* Delegate = CategoryToCollectFunction.Find(CategoryName))
	{
		if (Delegate->IsBound())
		{
			Delegate->Execute(AllCategoryItems);
		}
	}

	for (auto It = AllCategoryItems.CreateIterator(); It; ++It)
	{
		const TSharedPtr<FVoxelPlaceStampItem> Item = *It;
		if (!ensure(Item) ||
			!ensure(Item->Asset.IsValid()))
		{
			It.RemoveCurrent();
			continue;
		}

		Item->Setup();

		if (!ensureVoxelSlow(Item->ActorFactory))
		{
			It.RemoveCurrent();
			continue;
		}

		if (Item->Asset.IsValid())
		{
			TrackedAssetToItem.Add(Item->Asset.GetSoftObjectPath(), Item);
			if (Favorites.Contains(Item->Asset.GetSoftObjectPath()))
			{
				Item->bIsFavorite = true;
			}
		}
	}

	UpdateTags(true);
	UpdateFilteredItems();
}

void SVoxelPlaceStampsTab::UpdateTags(const bool bResetVisibleTags)
{
	TagsBox->ClearChildren();

	Tags = {};
	bool bAddEmptyTag = false;
	for (const TSharedPtr<FVoxelPlaceStampItem>& Item : AllCategoryItems)
	{
		Tags.Append(Item->UserTags);
		if (UseSystemTags())
		{
			Tags.Append(Item->SystemTags);
		}
		if (Item->UserTags.Num() == 0)
		{
			bAddEmptyTag = true;
		}
	}

	if (Tags.Num() > 0 &&
		bAddEmptyTag)
	{
		Tags.Add("Untagged");
	}

	if (bResetVisibleTags)
	{
		VisibleTags = Tags;
	}
	else
	{
		for (auto It = VisibleTags.CreateIterator(); It; ++It)
		{
			if (!Tags.Contains(*It))
			{
				It.RemoveCurrent();
			}
		}
	}

	if (Tags.Num() == 0)
	{
		TagsBox->SetVisibility(EVisibility::Collapsed);
		return;
	}

	TagsBox->SetVisibility(EVisibility::Visible);

	const auto AllTagsVisible = [this]
	{
		for (const FName ContentTag : Tags)
		{
			if (!VisibleTags.Contains(ContentTag))
			{
				return false;
			}
		}

		return true;
	};

	TagsBox->AddSlot()
	.Padding(5.f)
	[
		SNew(SBorder)
		.BorderImage(FVoxelEditorStyle::GetBrush("Graph.NewAssetDialog.FilterCheckBox.Border"))
		.ToolTipText(INVTEXT("Show all"))
		.Padding(3.f)
		[
			SNew(SCheckBox)
			.Style(FVoxelEditorStyle::Get(), "Graph.NewAssetDialog.FilterCheckBox")
			.BorderBackgroundColor_Lambda([=]() -> FSlateColor
			{
				return AllTagsVisible() ? FLinearColor::White : FSlateColor::UseForeground();
			})
			.IsChecked_Lambda([=]() -> ECheckBoxState
			{
				return AllTagsVisible() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([=, this](ECheckBoxState)
			{
				VisibleTags = AllTagsVisible() ? TSet<FName>() : Tags;
				UpdateFilteredItems();
			})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(2.f)
				[
					SNew(STextBlock)
					.Text(INVTEXT("Show all"))
					.ShadowOffset(0.f)
					.ColorAndOpacity_Lambda([=]() -> FSlateColor
					{
						return AllTagsVisible() ? FLinearColor::Black : FLinearColor::Gray;
					})
					.TextStyle(FVoxelEditorStyle::Get(), "Graph.NewAssetDialog.ActionFilterTextBlock")
				]
			]
		]
	];

	TArray<FName> SortedTags = Tags.Array();
	SortedTags.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });

	for (const FName ContentTag : SortedTags)
	{
		TagsBox->AddSlot()
		.Padding(3.f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush(TEXT("NoBorder")))
			.Padding(3.f)
			[
				SNew(SVoxelPlaceStampsTagCheckBox)
				.ContentTag(ContentTag)
				.IsChecked_Lambda([=, this]
				{
					return VisibleTags.Contains(ContentTag) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnContentTagClicked_Lambda([=, this](const bool bState)
				{
					if (bState)
					{
						VisibleTags.Add(ContentTag);
					}
					else
					{
						VisibleTags.Remove(ContentTag);
					}
					UpdateFilteredItems();
				})
				.OnContentTagShiftClicked_Lambda([=, this]
				{
					VisibleTags = { ContentTag };
					UpdateFilteredItems();
				})
			]
		];
	}
}

void SVoxelPlaceStampsTab::UpdateFilteredItems()
{
	FilteredCategoryItems = AllCategoryItems.FilterByPredicate([&](const TSharedPtr<FVoxelPlaceStampItem>& Item)
	{
		if (!Filter->PassesFilter(Item))
		{
			return false;
		}

		if (UseSystemTags())
		{
			for (const FName ContentTag : Item->SystemTags)
			{
				if (VisibleTags.Contains(ContentTag))
				{
					return true;
				}
			}
		}

		if (Item->UserTags.Num() == 0)
		{
			if (Tags.Num() > 0)
			{
				return VisibleTags.Contains(STATIC_FNAME("Untagged"));
			}

			return true;
		}

		for (const FName ContentTag : Item->UserTags)
		{
			if (VisibleTags.Contains(ContentTag))
			{
				return true;
			}
		}

		return false;
	});

	TileView->RequestListRefresh();

	if (FilteredCategoryItems.Num() == 0)
	{
		WidgetSwitcher->SetActiveWidgetIndex(0);
		// TODO: Texts
		if (Tags.Num() > 0 &&
			VisibleTags.Num() == 0)
		{
			MessageTextBlock->SetText(INVTEXT("No available stamps without tags"));
		}
		else if (!SearchBox->GetText().IsEmpty())
		{
			MessageTextBlock->SetText(INVTEXT("No available stamps matching this search"));
		}
		else
		{
			MessageTextBlock->SetText(INVTEXT("No available stamps in this category"));
		}
		return;
	}

	WidgetSwitcher->SetActiveWidgetIndex(1);
}

void SVoxelPlaceStampsTab::UpdateFavorites()
{
	TArray<FString> FavoriteStrings;

	const TUniquePtr<FStructProperty> StructProperty = FVoxelUtilities::MakeStructProperty(StaticStructFast<FSoftObjectPath>());
	for (const FSoftObjectPath& Data : Favorites)
	{
		FavoriteStrings.Add(Data.ToString());
	}

	GConfig->SetArray(TEXT("VoxelPlaceStamps"), TEXT("Favorites"), FavoriteStrings, GEditorPerProjectIni);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelPlaceStampsTab::OnAssetsAdded(const TArrayView<const FAssetData> AssetDatas)
{
	TFunction<TSharedPtr<FVoxelPlaceStampItem>(const FAssetData&)> ConstructFunction = nullptr;
	if (ActiveTabName == "Graphs")
	{
		ConstructFunction = &ConstructGraph;
	}
	else if (ActiveTabName == "Meshes")
	{
		ConstructFunction = &ConstructMesh;
	}
	else if (ActiveTabName == "Heightmaps")
	{
		ConstructFunction = &ConstructHeightmap;
	}
	else if (ActiveTabName == "Sculpts")
	{
		ConstructFunction = &ConstructSculpt;
	}
	else
	{
		return;
	}

	bool bHasNewItems = false;
	for (const FAssetData& Asset : AssetDatas)
	{
		// After renaming/moving asset, rename fixes the current item
		if (TrackedAssetToItem.Contains(Asset.GetSoftObjectPath()))
		{
			continue;
		}

		TSharedPtr<FVoxelPlaceStampItem> NewItem = ConstructFunction(Asset);
		if (!NewItem)
		{
			continue;
		}

		NewItem->Setup();

		AllCategoryItems.Add(NewItem);
		bHasNewItems = true;
	}

	if (!bHasNewItems)
	{
		return;
	}

	UpdateTags(false);
	UpdateFilteredItems();
}

void SVoxelPlaceStampsTab::OnAssetRenamed(const FAssetData& AssetData, const FString& OldName)
{
	const FSoftObjectPath OldPath = FSoftObjectPath(OldName);
	if (Favorites.Contains(OldPath))
	{
		Favorites.Remove(OldPath);
		Favorites.Add(AssetData.GetSoftObjectPath());
	}

	const TSharedPtr<FVoxelPlaceStampItem> Item = TrackedAssetToItem.FindRef(OldPath);
	if (!Item)
	{
		return;
	}

	Item->Name = AssetData.AssetName.ToString();
	Item->Asset = AssetData;
	TrackedAssetToItem.Remove(OldPath);
	TrackedAssetToItem.Add(AssetData.GetSoftObjectPath(), Item);
	UpdateFilteredItems();
}

void SVoxelPlaceStampsTab::OnAssetUpdated(const FAssetData& AssetData)
{
	const TSharedPtr<FVoxelPlaceStampItem> Item = TrackedAssetToItem.FindRef(AssetData.GetSoftObjectPath());
	if (!Item)
	{
		return;
	}

	Item->Name = AssetData.AssetName.ToString();
	Item->AssetThumbnail->RefreshThumbnail();
	Item->Asset = AssetData;

	if (Item->GetDescription.IsBound())
	{
		Item->Description = Item->GetDescription.Execute();
	}

	if (Item->GetTags.IsBound())
	{
		Item->UserTags = Item->GetTags.Execute();
		UpdateTags(false);
	}

	UpdateFilteredItems();
}

void SVoxelPlaceStampsTab::OnAssetRemoved(const FAssetData& AssetData)
{
	const FSoftObjectPath AssetPath = AssetData.GetSoftObjectPath();
	Favorites.Remove(AssetPath);

	const TSharedPtr<FVoxelPlaceStampItem> Item = TrackedAssetToItem.FindRef(AssetPath);
	if (!Item)
	{
		return;
	}

	TrackedAssetToItem.Remove(AssetPath);
	AllCategoryItems.Remove(Item);

	UpdateTags(false);
	UpdateFilteredItems();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<FVoxelPlaceStampItem> SVoxelPlaceStampsTab::ConstructGraph(const FAssetData& AssetData)
{
	const UClass* Class = AssetData.GetClass();
	if (!Class)
	{
		return nullptr;
	}

	if (!Class->IsChildOf<UVoxelGraph>())
	{
		return nullptr;
	}

	const TSharedRef<FVoxelPlaceStampItem> NewItem = MakeShared<FVoxelPlaceStampItem>();
	NewItem->Name = AssetData.AssetName.ToString();
	NewItem->AssetThumbnail = MakeShared<FAssetThumbnail>(AssetData, 128.f, 128.f, FVoxelEditorUtilities::GetThumbnailPool());
	NewItem->GetDescription = MakeWeakPtrDelegate(NewItem, [WeakItem = MakeWeakPtr(NewItem)]() -> FString
	{
		const TSharedPtr<FVoxelPlaceStampItem> Item = WeakItem.Pin();
		if (!Item)
		{
			return {};
		}

		return Item->Asset.GetTagValueRef<FString>(GET_MEMBER_NAME_STRING_CHECKED(UVoxelGraph, Description));
	});
	NewItem->GetTags = MakeWeakPtrDelegate(NewItem, [WeakItem = MakeWeakPtr(NewItem)]() -> TSet<FName>
	{
		const TSharedPtr<FVoxelPlaceStampItem> Item = WeakItem.Pin();
		if (!Item)
		{
			return {};
		}

		TSet<FName> Result;

		FVoxelUtilities::PropertyFromText_Direct(
			FindFPropertyChecked(UVoxelGraph, GraphTags),
			Item->Asset.GetTagValueRef<FString>(GET_MEMBER_NAME_STRING_CHECKED(UVoxelGraph, GraphTags)),
			reinterpret_cast<void*>(&Result),
			nullptr);

		return Result;
	});

	NewItem->SystemTags = { "Graph" };
	NewItem->Asset = AssetData;

	NewItem->Type = INLINE_LAMBDA
	{
		FString TypeName = AssetData.GetClass()->GetDisplayNameText().ToString();
		TypeName.RemoveFromStart("Voxel");
		return TypeName;
	};

	return NewItem;
}

TSharedPtr<FVoxelPlaceStampItem> SVoxelPlaceStampsTab::ConstructMesh(const FAssetData& AssetData)
{
	const UClass* Class = AssetData.GetClass();
	if (!Class)
	{
		return nullptr;
	}

	if (!Class->IsChildOf<UVoxelStaticMesh>())
	{
		return nullptr;
	}


	const TSharedRef<FVoxelPlaceStampItem> NewItem = MakeShared<FVoxelPlaceStampItem>();
	NewItem->Name = AssetData.AssetName.ToString();
	NewItem->AssetThumbnail = MakeShared<FAssetThumbnail>(AssetData, 128.f, 128.f, FVoxelEditorUtilities::GetThumbnailPool());

	NewItem->GetTags = MakeWeakPtrDelegate(NewItem, [WeakItem = MakeWeakPtr(NewItem)]() -> TSet<FName>
	{
		const TSharedPtr<FVoxelPlaceStampItem> Item = WeakItem.Pin();
		if (!Item)
		{
			return {};
		}

		TSet<FName> Result;

		FVoxelUtilities::PropertyFromText_Direct(
			FindFPropertyChecked(UVoxelStaticMesh, Tags),
			Item->Asset.GetTagValueRef<FString>(GET_MEMBER_NAME_STRING_CHECKED(UVoxelStaticMesh, Tags)),
			reinterpret_cast<void*>(&Result),
			nullptr);

		return Result;
	});

	NewItem->SystemTags = { "Mesh" };
	NewItem->Asset = AssetData;

	NewItem->Type = "Mesh";

	return NewItem;
}

TSharedPtr<FVoxelPlaceStampItem> SVoxelPlaceStampsTab::ConstructHeightmap(const FAssetData& AssetData)
{
	const UClass* Class = AssetData.GetClass();
	if (!Class)
	{
		return nullptr;
	}

	if (!Class->IsChildOf<UVoxelHeightmap>())
	{
		return nullptr;
	}

	const TSharedRef<FVoxelPlaceStampItem> NewItem = MakeShared<FVoxelPlaceStampItem>();
	NewItem->Name = AssetData.AssetName.ToString();
	NewItem->AssetThumbnail = MakeShared<FAssetThumbnail>(AssetData, 128.f, 128.f, FVoxelEditorUtilities::GetThumbnailPool());

	NewItem->GetTags = MakeWeakPtrDelegate(NewItem, [WeakItem = MakeWeakPtr(NewItem)]() -> TSet<FName>
	{
		const TSharedPtr<FVoxelPlaceStampItem> Item = WeakItem.Pin();
		if (!Item)
		{
			return {};
		}

		TSet<FName> Result;

		FVoxelUtilities::PropertyFromText_Direct(
			FindFPropertyChecked(UVoxelHeightmap, Tags),
			Item->Asset.GetTagValueRef<FString>(GET_MEMBER_NAME_STRING_CHECKED(UVoxelHeightmap, Tags)),
			reinterpret_cast<void*>(&Result),
			nullptr);

		return Result;
	});

	NewItem->SystemTags = { "Heightmap" };
	NewItem->Asset = AssetData;

	NewItem->Type = "Heightmap";

	return NewItem;
}

TSharedPtr<FVoxelPlaceStampItem> SVoxelPlaceStampsTab::ConstructSculpt(const FAssetData& AssetData)
{
	const UClass* Class = AssetData.GetClass();
	if (!Class)
	{
		return nullptr;
	}

	if (!Class->IsChildOf<UScriptStruct>())
	{
		return nullptr;
	}

	const UScriptStruct* Struct = Cast<UScriptStruct>(AssetData.GetAsset());
	if (!Struct->IsChildOf<FVoxelHeightSculptStamp>() &&
		!Struct->IsChildOf<FVoxelVolumeSculptStamp>())
	{
		return nullptr;
	}

	const TSharedRef<FVoxelPlaceStampItem> NewItem = MakeShared<FVoxelPlaceStampItem>();
	NewItem->Name = Struct->GetDisplayNameText().ToString();
	NewItem->AssetThumbnail = MakeShared<FAssetThumbnail>(AssetData, 128.f, 128.f, FVoxelEditorUtilities::GetThumbnailPool());

	NewItem->Asset = AssetData;

	if (Struct->IsChildOf<FVoxelHeightSculptStamp>())
	{
		NewItem->SystemTags = { "Sculpt", "Height" };
	}
	else if (Struct->IsChildOf<FVoxelVolumeSculptStamp>())
	{
		NewItem->SystemTags = { "Sculpt", "Volume" };
	}
	else
	{
		ensure(false);
	}

	NewItem->Type = "Sculpt";

	return NewItem;
}

TSharedPtr<FVoxelPlaceStampItem> SVoxelPlaceStampsTab::ConstructShape(const FAssetData& AssetData)
{
	const UClass* Class = AssetData.GetClass();
	if (!Class)
	{
		return nullptr;
	}

	if (!Class->IsChildOf<UScriptStruct>())
	{
		return nullptr;
	}

	const UScriptStruct* Struct = Cast<UScriptStruct>(AssetData.GetAsset());
	if (!Struct->IsChildOf<FVoxelShape>())
	{
		return nullptr;
	}

	const TSharedRef<FVoxelPlaceStampItem> NewItem = MakeShared<FVoxelPlaceStampItem>();
	if (Struct->HasMetaData("ShortName"))
	{
		NewItem->Name = Struct->GetMetaData("ShortName");
	}
	else
	{
		NewItem->Name = Struct->GetDisplayNameText().ToString();
	}

	if (Struct->HasMetaData("Thumbnail"))
	{
		NewItem->BrushName = Struct->GetMetaData("Thumbnail");
	}
	else
	{
		NewItem->AssetThumbnail = MakeShared<FAssetThumbnail>(AssetData, 128.f, 128.f, FVoxelEditorUtilities::GetThumbnailPool());
	}
	NewItem->Asset = AssetData;

	NewItem->Type = "Shape";

	return NewItem;
}

void SVoxelPlaceStampsTab::FillFavoritesList()
{
	TArray<FString> FavoriteStrings;
	GConfig->GetArray(TEXT("VoxelPlaceStamps"), TEXT("Favorites"), FavoriteStrings, GEditorPerProjectIni);

	for (const FString& Data : FavoriteStrings)
	{
		Favorites.Add(FSoftObjectPath(Data));
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelPlaceStampsTab::CollectAllFavorites(TArray<TSharedPtr<FVoxelPlaceStampItem>>& OutItems)
{
	const IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	for (const FSoftObjectPath& Path : Favorites)
	{
		FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(Path);
		if (!AssetData.IsValid())
		{
			continue;
		}

		if (const TSharedPtr<FVoxelPlaceStampItem> Item = ConstructGraph(AssetData))
		{
			OutItems.Add(Item);
			continue;
		}
		if (const TSharedPtr<FVoxelPlaceStampItem> Item = ConstructMesh(AssetData))
		{
			OutItems.Add(Item);
			continue;
		}
		if (const TSharedPtr<FVoxelPlaceStampItem> Item = ConstructHeightmap(AssetData))
		{
			OutItems.Add(Item);
			continue;
		}
		if (const TSharedPtr<FVoxelPlaceStampItem> Item = ConstructSculpt(AssetData))
		{
			OutItems.Add(Item);
			continue;
		}
		if (const TSharedPtr<FVoxelPlaceStampItem> Item = ConstructShape(AssetData))
		{
			OutItems.Add(Item);
			continue;
		}

		ensure(false);
	}
}

bool SVoxelPlaceStampsTab::UseSystemTags() const
{
	return
		ActiveTabName == STATIC_FNAME("Favorites") ||
		ActiveTabName == STATIC_FNAME("Recent");
}

void SVoxelPlaceStampsTab::CollectAllRecent(TArray<TSharedPtr<FVoxelPlaceStampItem>>& OutItems)
{
	if (!IPlacementModeModule::IsAvailable())
	{
		return;
	}

	const IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	for (const FActorPlacementInfo& Data : IPlacementModeModule::Get().GetRecentlyPlaced())
	{
		FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(Data.ObjectPath);
		if (!AssetData.IsValid())
		{
			continue;
		}

		if (const TSharedPtr<FVoxelPlaceStampItem> Item = ConstructGraph(AssetData))
		{
			OutItems.Add(Item);
			continue;
		}
		if (const TSharedPtr<FVoxelPlaceStampItem> Item = ConstructMesh(AssetData))
		{
			OutItems.Add(Item);
			continue;
		}
		if (const TSharedPtr<FVoxelPlaceStampItem> Item = ConstructHeightmap(AssetData))
		{
			OutItems.Add(Item);
			continue;
		}
		if (const TSharedPtr<FVoxelPlaceStampItem> Item = ConstructSculpt(AssetData))
		{
			OutItems.Add(Item);
			continue;
		}
		if (const TSharedPtr<FVoxelPlaceStampItem> Item = ConstructShape(AssetData))
		{
			OutItems.Add(Item);
			continue;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FLinearColor SVoxelPlaceStampsTab::GetStampTagColor(const FName StampTag)
{
	if (StampTag == STATIC_FNAME("Untagged"))
	{
		return FLinearColor::Black;
	}

	const uint32 Hash = FCrc::StrCrc32(*StampTag.ToString());
	const FLinearColor TagColor =  FLinearColor::MakeRandomSeededColor(Hash);
	return TagColor;
}

TSharedPtr<SVoxelPlaceStampsTab> SVoxelPlaceStampsTab::GetDrawerWidget()
{
	static TSharedPtr<SVoxelPlaceStampsTab> Drawer;
	if (!Drawer)
	{
		Drawer = SNew(SVoxelPlaceStampsTab, nullptr);
	}

	return Drawer;
}

void SVoxelPlaceStampsTab::RegisterDrawer()
{
	UStatusBarSubsystem* StatusBarSubsystem = GEditor->GetEditorSubsystem<UStatusBarSubsystem>();
	if (!StatusBarSubsystem)
	{
		return;
	}

	FWidgetDrawerConfig Config("PlaceVoxelStamps");
	Config.ButtonText = INVTEXT("Voxel Stamps");
	Config.Icon = FAppStyle::GetBrush("LevelEditor.Tabs.PlacementBrowser");
	Config.ToolTipText = INVTEXT("Toggles stamps drawer, from which stamps can be placed into world.");
	Config.GetDrawerContentDelegate.BindLambda([]() -> TSharedRef<SWidget>
	{
		return SVoxelPlaceStampsTab::GetDrawerWidget().ToSharedRef();
	});
	Config.OnDrawerOpenedDelegate.BindLambda([](FName StatusBarName)
	{
		SVoxelPlaceStampsTab::GetDrawerWidget()->FocusSearchBox();
	});

	StatusBarSubsystem->RegisterDrawer("LevelEditor.StatusBar", MoveTemp(Config), 1);
}

void SVoxelPlaceStampsTab::UnregisterDrawer()
{
	UStatusBarSubsystem* StatusBarSubsystem = GEditor->GetEditorSubsystem<UStatusBarSubsystem>();
	if (!StatusBarSubsystem)
	{
		return;
	}

	StatusBarSubsystem->UnregisterDrawer("LevelEditor.StatusBar", "PlaceVoxelStamps");
}