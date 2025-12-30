// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Volume/VoxelPaintVolumeTool.h"
#include "Sculpt/Volume/VoxelPaintVolumeModifier.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInterface.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

void UVoxelPaintVolumeTool::Enter()
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

void UVoxelPaintVolumeTool::Exit()
{
	VOXEL_FUNCTION_COUNTER();

	Super::Exit();
}

void UVoxelPaintVolumeTool::Tick()
{
	VOXEL_FUNCTION_COUNTER();

	Super::Tick();

	check(PreviewActor);
	PreviewActor->SetActorLocation(GetHitLocation());

	PreviewActor->SetActorScale3D(FVector(Radius / 50.f));

	UMaterialInstanceDynamic* MaterialInstance = PreviewMaterial;
	Brush.UpdateMaterial(MaterialInstance, Radius, GetHitNormal(), GetStrokeDirection());
	if (PreviewMaterial != MaterialInstance)
	{
		PreviewMaterial = MaterialInstance;
		PreviewActor->GetStaticMeshComponent()->SetMaterial(0, PreviewMaterial);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<FVoxelVolumeModifier> UVoxelPaintVolumeTool::GetModifier(const float StrengthMultiplier) const
{
	const TSharedRef<FVoxelPaintVolumeModifier> Modifier = MakeShared<FVoxelPaintVolumeModifier>();
	Modifier->Center = GetHitLocation();
	Modifier->SurfaceTypeToPaint = SurfaceTypeToPaint.SurfaceType;
	Modifier->MetadatasToPaint = MetadatasToPaint;
	Modifier->Radius = Radius;
	Modifier->Strength = Strength * StrengthMultiplier;
	Modifier->Mode = InverseAction() ? EVoxelSculptMode::Remove : EVoxelSculptMode::Add;
	Modifier->Brush = Brush.GetBrush(GetHitNormal(), GetStrokeDirection());
	return Modifier;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void UVoxelPaintVolumeTool::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	MetadatasToPaint.Fixup();
}
#endif