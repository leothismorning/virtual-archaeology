// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelToolEdMode.h"
#include "VoxelToolToolkit.h"
#include "VoxelSculptChanges.h"
#include "VoxelStampComponent.h"
#include "VoxelSurfacePickerEdMode.h"
#include "Sculpt/VoxelSculptActorBase.h"
#include "Sculpt/Height/VoxelHeightTool.h"
#include "Sculpt/Volume/VoxelVolumeTool.h"
#include "Sculpt/Height/VoxelHeightSculptData.h"
#include "Sculpt/Height/VoxelHeightSculptActor.h"
#include "Sculpt/Volume/VoxelVolumeSculptData.h"
#include "Sculpt/Volume/VoxelVolumeSculptActor.h"

#include "SceneView.h"
#include "Misc/ITransaction.h"
#include "EditorModeManager.h"
#include "EditorViewportClient.h"
#include "Toolkits/ToolkitManager.h"

VOXEL_CONSOLE_VARIABLE(
	VOXELEDITOR_API,
	bool, GVoxelSculptOneSculptPerClick, false,
	"voxel.sculpt.OneSculptPerClick",
	"If true, will ensure Sculpt is only called once per click. Useful to debug sculpting.");

VOXEL_RUN_ON_STARTUP_EDITOR()
{
	FEditorModeRegistry::Get().RegisterMode<FVoxelToolEdMode>(
		"VoxelToolEdMode",
		INVTEXT("Voxel Sculpt"),
		FSlateIcon(FVoxelEditorStyle::GetStyleSetName(), "VoxelIcon"),
		true
	);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelToolEdMode::Enter()
{
	FEdMode::Enter();

	if (!Toolkit)
	{
		Toolkit = MakeShared<FVoxelToolToolkit>(SharedThis(this));
		Toolkit->Init(Owner->GetToolkitHost());
	}

	bReinitializeTools = true;
}

void FVoxelToolEdMode::Exit()
{
	FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
	Toolkit.Reset();

	if (ActiveTool)
	{
		ActiveTool->CallExit();
		ActiveTool = {};
	}

	FEdMode::Exit();
}

void FVoxelToolEdMode::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdMode::AddReferencedObjects(Collector);

	// TODO SAVE TOOLS This is preventing GC when opening a map, causing crash
	Collector.AddReferencedObject(ActiveTool);

	for (auto& It : ClassToTool)
	{
		Collector.AddReferencedObject(It.Value);
	}
}

void FVoxelToolEdMode::Tick(FEditorViewportClient* ViewportClient, const float DeltaTime)
{
	FEdMode::Tick(ViewportClient, DeltaTime);

	if (!ActiveTool)
	{
		return;
	}

	if (!ActiveTool->Initialized())
	{
		ActiveTool->CallEnter(*GetWorld());
	}

	ActiveTool->CallTick(LastRay, FSlateApplication::Get().GetModifierKeys().IsShiftDown());

	if (bIsClicking)
	{
		DoEdit();
	}
}

bool FVoxelToolEdMode::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	if (!ActiveTool)
	{
		return false;
	}

	if (FVoxelSurfacePickerEdMode::IsSurfaceTypePickerActive())
	{
		return false;
	}

	if (Key != EKeys::LeftMouseButton)
	{
		if (Key == EKeys::RightMouseButton ||
			Key == EKeys::MiddleMouseButton)
		{
			if (ActiveTool)
			{
				ActiveTool->HideTool(Event == IE_Pressed);
				StopSculpting();
			}
		}
		return false;
	}

	switch (Event)
	{
	case IE_Pressed:
	{
		if (!Viewport->KeyState(EKeys::MiddleMouseButton) &&
			!Viewport->KeyState(EKeys::RightMouseButton) &&
			!IsAltDown(Viewport))
		{
			StartSculpting();
			return true;
		}
		break;
	}
	case IE_Released: StopSculpting(); break;
	default: break;
	}

	return false;
}

bool FVoxelToolEdMode::MouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y)
{
	LastRay = ComputeRay(ViewportClient, x, y);
	return FEdMode::MouseMove(ViewportClient, Viewport, x, y);
}

bool FVoxelToolEdMode::CapturedMouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY)
{
	LastRay = ComputeRay(InViewportClient, InMouseX, InMouseY);
	return true;
}

UVoxelTool* FVoxelToolEdMode::GetTool(const TSubclassOf<UVoxelTool>& Class)
{
	if (!Class)
	{
		return nullptr;
	}

	if (UVoxelTool* FoundTool = ClassToTool.FindRef(Class))
	{
		return FoundTool;
	}

	UVoxelTool* NewTool = NewObject<UVoxelTool>(GetTransientPackage(), Class);
	ClassToTool.Add_EnsureNew(Class, NewTool);
	return NewTool;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelToolEdMode::SetSculptActor(AVoxelSculptActorBase* NewSculptActor)
{
	VOXEL_FUNCTION_COUNTER();

	if (AVoxelSculptActorBase* OldSculptActor = WeakSculptActor.Resolve())
	{
		OldSculptActor->ClearSculptCache();
	}

	WeakSculptActor = NewSculptActor;
}

void FVoxelToolEdMode::SwitchSculptActor(AVoxelSculptActorBase* NewSculptActor)
{
	SetSculptActor(NewSculptActor);

	if (Toolkit)
	{
		StaticCastSharedPtr<FVoxelToolToolkit>(Toolkit)->SwitchSculptActor(NewSculptActor);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

const TVoxelMap<TSubclassOf<UVoxelTool>, TObjectPtr<UVoxelTool>>& FVoxelToolEdMode::GetClassToTool()
{
	if (bReinitializeTools ||
		ClassToTool.Num() == 0)
	{
		InitializeTools();
	}

	return ClassToTool;
}

void FVoxelToolEdMode::InitializeTools()
{
	TArray<TSubclassOf<UVoxelTool>> SculptClasses = GetDerivedClasses<UVoxelTool>();
	SculptClasses.Sort([](const TSubclassOf<UVoxelTool>& A, const TSubclassOf<UVoxelTool>& B)
	{
		return A->GetIntMetaData(STATIC_FNAME("Order")) < B->GetIntMetaData(STATIC_FNAME("Order"));
	});

	for (const TSubclassOf<UVoxelTool>& Class : SculptClasses)
	{
		if (Class->HasAnyClassFlags(CLASS_Abstract))
		{
			continue;
		}

		if (!ActiveToolClass)
		{
			ActiveToolClass = Class;
		}

		if (!ClassToTool.Contains(Class))
		{
			UVoxelTool* Tool = NewObject<UVoxelTool>(GetTransientPackage(), Class);
			ClassToTool.Add_CheckNew(Class, Tool);
		}
	}
}

void FVoxelToolEdMode::StartSculpting()
{
	//ensureVoxelSlow(!bIsClicking);
	bIsClicking = true;

	//ensureVoxelSlow(!Change);

	if (ActiveTool)
	{
		ActiveTool->OnEditBegin();
		ActiveTool->CallTick(LastRay, FSlateApplication::Get().GetModifierKeys().IsShiftDown());

		// Default to 30Hz for the first edit
		LastEditTime = FPlatformTime::Seconds() - 1.f / 30.f;

		DoEdit();

		if (GVoxelSculptOneSculptPerClick)
		{
			StopSculpting();
		}
	}
}

void FVoxelToolEdMode::StopSculpting()
{
	if (!bIsClicking)
	{
		return;
	}

	bIsClicking = false;

	if (ActiveTool)
	{
		ActiveTool->OnEditEnd();
	}

	ON_SCOPE_EXIT
	{
		Change.Reset();
	};

	if (!ActiveTool)
	{
		return;
	}

	AVoxelSculptActorBase* SculptActor = WeakSculptActor.Resolve();
	if (!SculptActor)
	{
		return;
	}

	FVoxelTransaction Transaction(ActiveTool, "Sculpt voxels");

	GUndo->StoreUndo(
		SculptActor->GetRootComponent(),
		MoveTemp(Change));
}

void FVoxelToolEdMode::DoEdit()
{
	VOXEL_FUNCTION_COUNTER();

	if (!Future.IsComplete())
	{
		return;
	}

	double DeltaTime = FPlatformTime::Seconds() - LastEditTime;
	if (DeltaTime < 1.f / 60.f)
	{
		// Don't edit at more than 60Hz
		return;
	}

	// Assume a base sculpting speed of 30Hz
	const float StrengthMultiplier = FMath::Min(DeltaTime, 0.1f) * 30.f;

	AVoxelSculptActorBase* SculptActor = WeakSculptActor.Resolve();
	if (!ensure(ActiveTool) ||
		!ensure(SculptActor))
	{
		return;
	}

	if (UVoxelHeightTool* Tool = Cast<UVoxelHeightTool>(ActiveTool))
	{
		AVoxelHeightSculptActor& Actor = *CastChecked<AVoxelHeightSculptActor>(SculptActor);

		if (!Tool->PrepareModifierData())
		{
			return;
		}

		const TSharedPtr<FVoxelHeightModifier> Modifier = Tool->GetModifier(StrengthMultiplier);
		if (!ensureVoxelSlow(Modifier))
		{
			return;
		}

		if (!Change)
		{
			Change = MakeUnique<FVoxelHeightSculptChange>(Actor.GetStamp()->GetData()->GetInnerData());
		}

		ensure(Future.IsComplete());
		Future = Actor.ApplyModifier(Modifier.ToSharedRef()).Then_GameThread(MakeWeakPtrLambda(this, [this]
		{
			LastEditTime = FPlatformTime::Seconds();
		}));

		return;
	}

	if (UVoxelVolumeTool* Tool = Cast<UVoxelVolumeTool>(ActiveTool))
	{
		AVoxelVolumeSculptActor& Actor = *CastChecked<AVoxelVolumeSculptActor>(SculptActor);

		if (!Tool->PrepareModifierData())
		{
			return;
		}

		const TSharedPtr<FVoxelVolumeModifier> Modifier = Tool->GetModifier(StrengthMultiplier);
		if (!ensureVoxelSlow(Modifier))
		{
			return;
		}

		if (!Change)
		{
			Change = MakeUnique<FVoxelVolumeSculptChange>(Actor.GetStamp()->GetData()->GetInnerData());
		}

		ensure(Future.IsComplete());
		Future = Actor.ApplyModifier(Modifier.ToSharedRef()).Then_GameThread(MakeWeakPtrLambda(this, [this]
		{
			LastEditTime = FPlatformTime::Seconds();
		}));

		return;
	}

	ensure(false);
}

FRay FVoxelToolEdMode::ComputeRay(FEditorViewportClient* ViewportClient, const int32 X, const int32 Y)
{
	FSceneViewFamilyContext ViewFamily(
		FSceneViewFamily::ConstructionValues(
		ViewportClient->Viewport,
		ViewportClient->GetScene(),
		ViewportClient->EngineShowFlags)
		.SetRealtimeUpdate(ViewportClient->IsRealtime()));

	const FSceneView* View = ViewportClient->CalcSceneView(&ViewFamily);
	const FViewportCursorLocation MouseViewportRay(View, ViewportClient, X, Y);
	const FVector BrushTraceDirection = MouseViewportRay.GetDirection();

	FVector BrushTraceStart = MouseViewportRay.GetOrigin();
	if (ViewportClient->IsOrtho())
	{
		BrushTraceStart += -WORLD_MAX * BrushTraceDirection;
	}

	return FRay(BrushTraceStart, BrushTraceDirection);
}