// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelLayers.h"
#include "VoxelToolEdMode.h"
#include "VoxelSculptChanges.h"
#include "VoxelStampCustomization.h"
#include "Sculpt/Volume/VoxelVolumeSculptData.h"
#include "Sculpt/Volume/VoxelVolumeSculptStamp.h"
#include "Sculpt/Volume/VoxelVolumeSculptActor.h"
#include "EditorModes.h"
#include "Misc/ITransaction.h"
#include "EditorModeManager.h"

class FVoxelVolumeSculptStampCustomization : public FVoxelStampCustomization
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
						FVoxelEditorUtilities::ForeachData<FVoxelVolumeSculptStamp>(PropertyHandle, [&](FVoxelVolumeSculptStamp& SculptStamp, UObject* Outer)
						{
							if (AVoxelVolumeSculptActor* SculptActor = Outer->GetTypedOuter<AVoxelVolumeSculptActor>())
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

					FVoxelEditorUtilities::ForeachData<FVoxelVolumeSculptStamp>(PropertyHandle, [&](FVoxelVolumeSculptStamp& SculptStamp, UObject* Outer)
					{
						if (const AVoxelVolumeSculptActor* SculptActor = Outer->GetTypedOuter<AVoxelVolumeSculptActor>())
						{
							GUndo->StoreUndo(
								SculptActor->GetRootComponent(),
								MakeUnique<FVoxelVolumeSculptChange>(SculptActor->GetStamp()->GetData()->GetInnerData()));
						}

						SculptStamp.ClearSculptData();
					});

					return FReply::Handled();
				})
			]
		];
	}
};

DEFINE_VOXEL_STRUCT_LAYOUT(FVoxelVolumeSculptStamp, FVoxelVolumeSculptStampCustomization);