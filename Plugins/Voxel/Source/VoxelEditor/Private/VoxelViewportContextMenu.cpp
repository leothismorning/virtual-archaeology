// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"

#include "VoxelQuery.h"
#include "VoxelState.h"
#include "VoxelWorld.h"
#include "VoxelLayers.h"
#include "VoxelRuntime.h"
#include "VoxelStampDebug.h"
#include "VoxelEditorSettings.h"
#include "SVoxelStampDeltaList.h"
#include "Collision/VoxelCollisionChannels.h"

#include "ToolMenus.h"
#include "LevelEditorViewport.h"
#include "LevelEditorMenuContext.h"

class FVoxelViewportContextMenu
{
public:
	static void OnConstructDynamicSection(UToolMenu* ToolMenu)
	{
		if (!GetDefault<UVoxelEditorSettings>()->bEnableViewportContextMenuActions)
		{
			return;
		}

		const ULevelEditorContextMenuContext* LevelEditorContext = ToolMenu->FindContext<ULevelEditorContextMenuContext>();
		if (!LevelEditorContext ||
			!LevelEditorContext->HitProxyActor.IsValid())
		{
			return;
		}

		const AVoxelWorld* VoxelWorld = Cast<AVoxelWorld>(LevelEditorContext->HitProxyActor.Get());
		if (!VoxelWorld)
		{
			return;
		}

		const TSharedPtr<FVoxelRuntime> Runtime = VoxelWorld->GetRuntime();
		if (!Runtime)
		{
			return;
		}

		const TSharedPtr<FVoxelState> State = Runtime->GetState();
		if (!State)
		{
			return;
		}

		FVector Start;
		FVector End;
		if (!ensure(FVoxelEditorUtilities::GetRayInfo(GCurrentLevelEditingViewportClient, Start, End)))
		{
			return;
		}

		const UWorld* World = GCurrentLevelEditingViewportClient->GetWorld();
		if (!ensure(World))
		{
			return;
		}

		FHitResult HitResult;
		if (!World->LineTraceSingleByChannel(HitResult, Start, End, ECC_VoxelEditor))
		{
			return;
		}

		const FVector WorldPosition = HitResult.Location;
		const FVector LocalPosition = VoxelWorld->ActorToWorld().InverseTransformPosition(WorldPosition);

		const TVoxelArray<FVoxelStampDelta> StampDeltas = State->Layers->GetStampDeltas(
			State->Config->LayerToRender,
			LocalPosition,
			0);

		if (StampDeltas.Num() == 0)
		{
			return;
		}

		FToolMenuSection& Section = ToolMenu->AddSection("VoxelStamps", INVTEXT("Voxel Stamps"));

		Section.AddSubMenu(
			"VoxelStamps",
			FText::FromString("Voxel Stamps [" + LexToString(StampDeltas.Num()) + "]"),
			{},
			FNewToolMenuDelegate::CreateLambda([StampDeltas ](UToolMenu* Menu)
			{
				Menu->bSearchable = false;

				FToolMenuSection& Section = Menu->AddSection("Stamps");
				const TSharedRef<SWidget> Widget =
					SNew(SBox)
					.MaxDesiredHeight(400.f)
					.MinDesiredWidth(500.f)
					[
						SNew(SVoxelStampDeltaList)
						.StampDeltas(StampDeltas)
					];

				Section.AddEntry(FToolMenuEntry::InitWidget("PickStamp", Widget, FText::GetEmpty(), false, false, true));
			}),
			false,
			FSlateIcon(FVoxelEditorStyle::GetStyleSetName(), "VoxelIcon"));

#if 0
		Section.AddMenuEntry(
			"VoxelStampDebug",
			INVTEXT("Debug Voxel Stamps"),
			{},
			FSlateIcon(FVoxelEditorStyle::GetStyleSetName(), "VoxelIcon"),
			FToolUIActionChoice(MakeLambdaDelegate([=]
			{
				FVoxelStampDebug::Open(*State, WorldPosition);
			})));
#endif
	}
};

VOXEL_RUN_ON_STARTUP_EDITOR()
{
	UToolMenu* ComponentMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.ActorContextMenu");
	if (!ComponentMenu)
	{
		return;
	}

	ComponentMenu->AddDynamicSection(
		"Voxel",
		FNewToolMenuDelegate::CreateLambda([](UToolMenu* ToolMenu)
		{
			FVoxelViewportContextMenu::OnConstructDynamicSection(ToolMenu);
		}),
		FToolMenuInsert("ActorGeneral", EToolMenuInsertType::Before));
}