// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelLayers.h"
#include "VoxelToolEdMode.h"
#include "VoxelSculptChanges.h"
#include "VoxelStampComponent.h"
#include "VoxelStampCustomization.h"
#include "Sculpt/Height/VoxelHeightSculptData.h"
#include "Sculpt/Height/VoxelHeightSculptActor.h"
#include "Sculpt/Height/VoxelHeightSculptStamp.h"
#include "EditorModes.h"
#include "Misc/ITransaction.h"
#include "EditorModeManager.h"

class FVoxelHeightSculptStampCustomization : public FVoxelStampCustomization
{
public:
	virtual void CustomizeChildren(
		const TSharedRef<IPropertyHandle> PropertyHandle,
		IDetailChildrenBuilder& ChildBuilder,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override
	{
		FVoxelStampCustomization::CustomizeChildren(PropertyHandle, ChildBuilder, CustomizationUtils);

		ChildBuilder.AddCustomRow(INVTEXT("Sculpt Data"))
		.WholeRowContent()
		.HAlign(HAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(0.f, 2.f)
			.AutoWidth()
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Text_Lambda([]
				{
					return
						GLevelEditorModeTools().IsModeActive("VoxelToolEdMode")
						? INVTEXT("Exit Sculpt Mode")
						: INVTEXT("Enter Sculpt Mode");
				})
				.OnClicked_Lambda([PropertyHandle]
				{
					if (GLevelEditorModeTools().IsModeActive("VoxelToolEdMode"))
					{
						GLevelEditorModeTools().DeactivateMode("VoxelToolEdMode");
						GLevelEditorModeTools().ActivateDefaultMode();
						return FReply::Handled();
					}

					GLevelEditorModeTools().DeactivateAllModes();
					GLevelEditorModeTools().ActivateMode("VoxelToolEdMode");

					if (FVoxelToolEdMode* EdMode = static_cast<FVoxelToolEdMode*>(GLevelEditorModeTools().GetActiveMode("VoxelToolEdMode")))
					{
						FVoxelEditorUtilities::ForeachData<FVoxelHeightSculptStamp>(PropertyHandle, [&](FVoxelHeightSculptStamp& SculptStamp, UObject* Outer)
						{
							if (AVoxelHeightSculptActor* SculptActor = Outer->GetTypedOuter<AVoxelHeightSculptActor>())
							{
								EdMode->SwitchSculptActor(SculptActor);
							}
						});
					}
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot()
			.Padding(0.f, 2.f)
			.AutoWidth()
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Text(INVTEXT("Clear Sculpt Data"))
				.OnClicked_Lambda([PropertyHandle]
				{
					FScopedTransaction Transaction(INVTEXT("Clear Sculpt Data"));

					FVoxelEditorUtilities::ForeachData<FVoxelHeightSculptStamp>(PropertyHandle, [&](FVoxelHeightSculptStamp& SculptStamp, UObject* Outer)
					{
						if (const AVoxelHeightSculptActor* SculptActor = Outer->GetTypedOuter<AVoxelHeightSculptActor>())
						{
							GUndo->StoreUndo(
								SculptActor->GetRootComponent(),
								MakeUnique<FVoxelHeightSculptChange>(SculptActor->GetStamp()->GetData()->GetInnerData()));
						}

						SculptStamp.ClearSculptData();
					});

					return FReply::Handled();
				})
			]
		];
	}
};

DEFINE_VOXEL_STRUCT_LAYOUT(FVoxelHeightSculptStamp, FVoxelHeightSculptStampCustomization);