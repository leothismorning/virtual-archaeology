// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Volume/VoxelResetTool.h"
#include "VoxelWorld.h"
#include "VoxelConfig.h"
#include "VoxelRuntime.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInterface.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

void UVoxelResetTool::Enter()
{
	VOXEL_FUNCTION_COUNTER();

	Super::Enter();

	PreviewActor = SpawnActor<AStaticMeshActor>();
	if (!ensure(PreviewActor))
	{
		return;
	}

	PreviewActor->SetActorEnableCollision(false);

	UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	ensure(StaticMesh);
	PreviewActor->GetStaticMeshComponent()->SetStaticMesh(StaticMesh);

	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Voxel/Tools/M_ToolSphere.M_ToolSphere"));
	ensure(Material);

	PreviewMaterial = UMaterialInstanceDynamic::Create(Material, PreviewActor);
	PreviewMaterial->SetScalarParameterValue(STATIC_FNAME("Radius"), Radius);

	PreviewActor->GetStaticMeshComponent()->SetMaterial(0, PreviewMaterial);
}

void UVoxelResetTool::Exit()
{
	VOXEL_FUNCTION_COUNTER();

	Super::Exit();
}

void UVoxelResetTool::Tick()
{
	VOXEL_FUNCTION_COUNTER();

	Super::Tick();

	check(PreviewActor);
	PreviewActor->SetActorLocation(GetHitLocation());

	PreviewActor->SetActorScale3D(FVector(Radius / 50.f));
}

#if WITH_EDITOR
void UVoxelResetTool::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_STATIC(UVoxelResetTool, Radius))
	{
		PreviewMaterial->SetScalarParameterValue(STATIC_FNAME("Radius"), Radius);
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if 0 // TODO
void UVoxelResetTool::Edit(const TSharedRef<const FVoxelConfig>& Config)
{
	VOXEL_FUNCTION_COUNTER();

	const FVector HitPosition = Config->WorldToLocal.TransformPosition(GetHitLocation()) / Config->VoxelSize;
	const FVoxelIntBox Bounds = FVoxelIntBox::FromFloatBox_NoPadding(FVoxelBox(HitPosition)).Extend(FMath::CeilToInt((Radius / Config->VoxelSize) + 4));

	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

#if 0 // TODO
	if (!ensure(GetVoxelWorld()))
	{
		return;
	}

	const TSharedPtr<FVoxelRuntime> Runtime = GetVoxelWorld()->GetVoxelRuntime();
	if (!ensure(Runtime) ||
		!ensure(!Runtime->IsProcessingNewState()))
	{
		return;
	}

	const TSharedPtr<FVoxelState> State = Runtime->GetState();
	if (!ensure(State))
	{
		return;
	}

	InvalidatedBounds += Bounds;

	TVoxelArray<TSharedPtr<FVoxelSculptTree>> NewLODToTree;

	for (int32 LOD = 0; LOD < 20; LOD++)
	{
		const TSharedPtr<FVoxelSculptTree> OldTree = Runtime->GetState()->Config->SculptStorage->GetTree(LOD);
		if (!OldTree)
		{
			NewLODToTree.Add(nullptr);
			return;
		}

		// TODO HACK Should also update Octree
		TVoxelMap<FIntVector, TSharedPtr<const FVoxelSculptChunk>> KeyToChunk = OldTree->KeyToChunk;
		for (auto It = KeyToChunk.CreateIterator(); It; ++It)
		{
			if (Bounds.Intersect(FVoxelIntBox(It.Key()).Scale(FVoxelSculptChunk::ChunkSize << LOD)))
			{
				It.RemoveCurrent();
			}
		}

		NewLODToTree.Add(MakeShared<FVoxelSculptTree>(
			0,
			TVoxelFastOctree<>(OldTree->Octree),
			MoveTemp(KeyToChunk)));
	}

	const TSharedRef<FVoxelSculptStorage> Storage = MakeShared<FVoxelSculptStorage>(NewLODToTree);

	// TODO HACK: CreateChange is called after this function in FVoxelToolEdMode::StartSculpting because we assume the edit won't be instant
	Voxel::GameTask_Async([WeakVoxelWorld = MakeWeakObjectPtr(GetVoxelWorld()), Storage, Bounds]
	{
		if (ensure(WeakVoxelWorld.IsValid()))
		{
#if 0 // TODO
			WeakVoxelWorld->SetSculptStorage(Storage, Bounds);
#endif
		}
	});
#endif
}
#endif