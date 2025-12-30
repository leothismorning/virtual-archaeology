// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Volume/VoxelSmoothVolumeTool.h"
#include "Sculpt/Volume/VoxelSmoothVolumeModifier.h"
#include "VoxelConfig.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInterface.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

void UVoxelSmoothVolumeTool::Enter()
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
	PreviewActor->GetStaticMeshComponent()->SetMaterial(0, PreviewMaterial);
}

void UVoxelSmoothVolumeTool::Exit()
{
	VOXEL_FUNCTION_COUNTER();

	Super::Exit();
}

void UVoxelSmoothVolumeTool::Tick()
{
	VOXEL_FUNCTION_COUNTER();

	Super::Tick();

	PreviewActor->SetActorLocation(GetHitLocation());
	PreviewActor->SetActorScale3D(FVector(Radius / 50.f));

	UMaterialInstanceDynamic* Material = PreviewMaterial;
	Brush.UpdateMaterial(Material, Radius, GetHitNormal(), GetStrokeDirection());
	if (PreviewMaterial != Material)
	{
		PreviewMaterial = Material;
		PreviewActor->GetStaticMeshComponent()->SetMaterial(0, PreviewMaterial);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<FVoxelVolumeModifier> UVoxelSmoothVolumeTool::GetModifier(const float StrengthMultiplier) const
{
	const TSharedRef<FVoxelSmoothVolumeModifier> Modifier = MakeShared<FVoxelSmoothVolumeModifier>();
	Modifier->Center = GetHitLocation();
	Modifier->Radius = Radius;
	Modifier->Strength = -Strength * StrengthMultiplier;
	Modifier->Brush = Brush.GetBrush(GetHitNormal(), GetStrokeDirection());
	return Modifier;
}