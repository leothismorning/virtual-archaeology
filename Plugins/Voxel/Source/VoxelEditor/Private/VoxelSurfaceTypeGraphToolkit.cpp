// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelSurfaceTypeGraphToolkit.h"
#include "VoxelWorld.h"
#include "VoxelStampActor.h"
#include "VoxelGraphToolkit.h"
#include "Surface/VoxelSmartSurfaceType.h"
#include "Surface/VoxelSurfaceTypeGraph.h"
#include "SVoxelMaterialGraphShapeToolbar.h"

void FVoxelSurfaceTypeGraphViewportInterface::PopulateOverlay(const TSharedRef<SOverlay>& Overlay)
{
	FVoxelStampGraphViewportInterface::PopulateOverlay(Overlay);

	UVoxelSurfaceTypeGraph* MaterialGraph = Cast<UVoxelSurfaceTypeGraph>(GetGraph());
	if (!ensure(MaterialGraph))
	{
		return;
	}

	Overlay->AddSlot()
	.VAlign(VAlign_Bottom)
	[
		SNew(SVoxelMaterialGraphShapeToolbar)
		.Type_Lambda(MakeWeakObjectPtrLambda(MaterialGraph, [MaterialGraph]
		{
			return MaterialGraph->PreviewShape;
		}))
		.OnTypeChanged_Lambda(MakeWeakObjectPtrLambda(MaterialGraph, [this, MaterialGraph](const EVoxelSmartSurfacePreviewShape NewType)
		{
			FVoxelTransaction Transaction(MaterialGraph);

			MaterialGraph->PreviewShape = NewType;

			AVoxelStampActor* StampActor = WeakStampActor.Resolve();
			if (!StampActor)
			{
				return;
			}

			StampActor->SetStamp(FVoxelSmartSurfacePreviewShapeUtilities::MakeStamp(
				MaterialGraph->PreviewShape,
				MaterialGraph->GetPreviewSurface()));
		}))
	];
}

void FVoxelSurfaceTypeGraphViewportInterface::SetupVoxelWorld(AVoxelWorld& VoxelWorld) const
{
	VoxelWorld.VoxelSize = 5;
}

FVoxelStampRef FVoxelSurfaceTypeGraphViewportInterface::MakeStampRef(UVoxelGraph& Graph) const
{
	UVoxelSurfaceTypeGraph& MaterialGraph = CastChecked<UVoxelSurfaceTypeGraph>(Graph);

	return FVoxelSmartSurfacePreviewShapeUtilities::MakeStamp(
		MaterialGraph.PreviewShape,
		MaterialGraph.GetPreviewSurface());
}

TOptional<float> FVoxelSurfaceTypeGraphViewportInterface::GetBoundsSize(const FVoxelStampRef& StampRef) const
{
	return 200.f;
}

#if VOXEL_ENGINE_VERSION >= 506
void FVoxelSurfaceTypeGraphViewportInterface::ExtendToolbar(UToolMenu& ToolMenu)
{
	UVoxelSurfaceTypeGraph* MaterialGraph = Cast<UVoxelSurfaceTypeGraph>(GetGraph());
	if (!ensure(MaterialGraph))
	{
		return;
	}

	SVoxelMaterialGraphShapeToolbar::ExtendToolbar(
		ToolMenu,
		MakeWeakObjectPtrDelegate(MaterialGraph, [this, MaterialGraph](const EVoxelSmartSurfacePreviewShape NewType)
		{
			FVoxelTransaction Transaction(MaterialGraph);

			MaterialGraph->PreviewShape = NewType;

			AVoxelStampActor* StampActor = WeakStampActor.Resolve();
			if (!StampActor)
			{
				return;
			}

			StampActor->SetStamp(FVoxelSmartSurfacePreviewShapeUtilities::MakeStamp(
				MaterialGraph->PreviewShape,
				MaterialGraph->GetPreviewSurface()));
		}),
		MakeWeakObjectPtrDelegate(MaterialGraph, [MaterialGraph]
		{
			return MaterialGraph->PreviewShape;
		}));
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TArray<FVoxelToolkit::FMode> FVoxelSurfaceTypeGraphToolkit::GetModes() const
{
	TArray<FMode> Modes;
	{
		FMode Mode;
		Mode.Struct = FVoxelSurfaceTypeGraphPreviewToolkit::StaticStruct();
		Mode.Object = Asset;
		Mode.DisplayName = INVTEXT("Preview");
		Mode.Icon = FAppStyle::GetBrush("UMGEditor.SwitchToDesigner");
		Modes.Add(Mode);
	}
	{
		FMode Mode;
		Mode.Struct = FVoxelGraphToolkit::StaticStruct();
		Mode.Object = Asset;
		Mode.DisplayName = INVTEXT("Graph");
		Mode.Icon = FAppStyle::GetBrush("FullBlueprintEditor.SwitchToScriptingMode");
		Mode.ConfigureToolkit = [WeakAsset = MakeVoxelObjectPtr(Asset)](FVoxelToolkit& Toolkit)
		{
			CastStructChecked<FVoxelGraphToolkit>(Toolkit).ViewportInterface = MakeShared<FVoxelSurfaceTypeGraphViewportInterface>();
		};
		Modes.Add(Mode);
	}
	return Modes;
}

UScriptStruct* FVoxelSurfaceTypeGraphToolkit::GetDefaultMode() const
{
	return FVoxelGraphToolkit::StaticStruct();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelSurfaceTypeGraphPreviewToolkit::SetupPreview()
{
	ViewportInterface = MakeShared<FVoxelSurfaceTypeGraphViewportInterface>();
	ViewportInterface->SetupPreview(*Asset, GetSharedViewport());
}

FRotator FVoxelSurfaceTypeGraphPreviewToolkit::GetInitialViewRotation() const
{
	return ViewportInterface->GetInitialViewRotation();
}

TOptional<float> FVoxelSurfaceTypeGraphPreviewToolkit::GetInitialViewDistance() const
{
	return ViewportInterface->GetInitialViewDistance();
}

void FVoxelSurfaceTypeGraphPreviewToolkit::PostEditChange(const FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChange(PropertyChangedEvent);

	ViewportInterface->UpdateStamp();
}

TSharedPtr<SWidget> FVoxelSurfaceTypeGraphPreviewToolkit::GetMenuOverlay() const
{
	return FVoxelGraphToolkit::MakeMenuOverlay(Asset);
}