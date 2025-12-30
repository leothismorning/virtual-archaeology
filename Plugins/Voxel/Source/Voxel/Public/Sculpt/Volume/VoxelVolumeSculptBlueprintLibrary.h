// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Sculpt/VoxelSculptSave.h"
#include "Sculpt/Volume/VoxelPaintVolumeModifier.h"
#include "Sculpt/Volume/VoxelAngleVolumeModifier.h"
#include "Sculpt/Volume/VoxelFlattenVolumeModifier.h"
#include "Sculpt/Volume/VoxelGraphVolumeModifier.h"
#include "Sculpt/Volume/VoxelSmoothVolumeModifier.h"
#include "Sculpt/Volume/VoxelSphereVolumeModifier.h"
#include "Sculpt/Volume/VoxelSurfaceVolumeModifier.h"
#include "VoxelVolumeSculptBlueprintLibrary.generated.h"

UCLASS()
class VOXEL_API UVoxelVolumeSculptBlueprintLibrary  : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Voxel")
	static bool IsValidSave(FVoxelVolumeSculptSave Save);

	UFUNCTION(BlueprintPure, Category = "Voxel")
	static bool IsCompressedSave(FVoxelVolumeSculptSave Save);

	UFUNCTION(BlueprintPure, Category = "Voxel")
	static int64 GetSaveSize(FVoxelVolumeSculptSave Save);

public:
	// Clear all sculpt data
	UFUNCTION(BlueprintCallable, Category = "Voxel|Sculpt")
	static FVoxelFuture ClearSculptData(UPARAM(Required) AVoxelVolumeSculptActor* SculptActor);

	// Clear all sculpt cache
	// This will free up memory but subsequent edits might be slightly slower
	UFUNCTION(BlueprintCallable, Category = "Voxel|Sculpt")
	static void ClearSculptCache(UPARAM(Required) AVoxelVolumeSculptActor* SculptActor);

public:
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	static FVoxelFuture GetSave(
		FVoxelVolumeSculptSave& Save,
		UPARAM(Required) AVoxelVolumeSculptActor* SculptActor,
		bool bCompress = true);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	static FVoxelFuture LoadFromSave(
		UPARAM(Required) AVoxelVolumeSculptActor* SculptActor,
		FVoxelVolumeSculptSave Save);

private:
	static FVoxelFuture ApplyModifier(
		UPARAM(Required) AVoxelVolumeSculptActor* SculptActor,
		const TSharedRef<FVoxelVolumeModifier>& Modifier);

public:
	/**
	 * Paint a surface type on a sculpt stamp
	 * @param SculptActor			The sculpt actor to paint
	 * @param Center				Center, in world space
	 * @param Radius				Radius, in world space
	 * @param Strength				Strength, usually between 0 and 1
	 * @param Mode					Add or remove surface type
	 * @param SurfaceTypeToPaint	Surface type to paint
	 * @param MetadatasToPaint		Metadatas to paint
	 * @param Brush					Optional, used to apply a pattern to the paint
	 */
	UFUNCTION(BlueprintCallable, Category = "Voxel|Sculpt")
	static FVoxelFuture PaintSurface(
		UPARAM(Required) AVoxelVolumeSculptActor* SculptActor,
		const FVector Center,
		const float Radius = 500.f,
		const float Strength = 0.05f,
		const EVoxelSculptMode Mode = EVoxelSculptMode::Add,
		UVoxelSurfaceTypeInterface* SurfaceTypeToPaint = nullptr,
		const FVoxelMetadataOverrides MetadatasToPaint = FVoxelMetadataOverrides(),
		const FVoxelToolBrush Brush = FVoxelToolBrush())
	{
		VOXEL_FUNCTION_COUNTER();

		const TSharedRef<FVoxelPaintVolumeModifier> Modifier = MakeShared<FVoxelPaintVolumeModifier>();
		Modifier->Center = Center;
		Modifier->SurfaceTypeToPaint = SurfaceTypeToPaint;
		Modifier->MetadatasToPaint = MetadatasToPaint;
		Modifier->Radius = Radius;
		Modifier->Strength = Strength;
		Modifier->Mode = Mode;
		Modifier->Brush = Brush;
		return ApplyModifier(SculptActor, Modifier);
	}

public:
	/**
	 * Move a voxel surface to towards a plane. Useful to sculpt angles.
	 * @param SculptActor	The sculpt actor to sculpt
	 * @param Center		Center, in world space
	 * @param Radius		Radius, in world space
	 * @param Strength		Strength, usually between 0 and 1
	 * @param PlanePoint	A point on the plane to fit towards. Usually the click position.
	 * @param PlaneNormal	The plane normal that will decide the angle
	 * @param MergeMode		Merge mode
	 * @param Brush			Optional, used to apply a pattern
	 */
	UFUNCTION(BlueprintCallable, Category = "Voxel|Sculpt")
	static FVoxelFuture SculptAngle(
		UPARAM(Required) AVoxelVolumeSculptActor* SculptActor,
		const FVector Center,
		const float Radius = 500.f,
		const float Strength = 1.f,
		const FVector& PlanePoint = FVector(0, 0, 0),
		const FVector& PlaneNormal = FVector(0, 0, 1),
		const EVoxelSDFMergeMode MergeMode = EVoxelSDFMergeMode::Override,
		const FVoxelToolBrush Brush = FVoxelToolBrush())
	{
		VOXEL_FUNCTION_COUNTER();

		const TSharedRef<FVoxelAngleVolumeModifier> Modifier = MakeShared<FVoxelAngleVolumeModifier>();
		Modifier->Center = Center;
		Modifier->Radius = Radius;
		Modifier->Strength = Strength;
		Modifier->Plane = FPlane(PlanePoint, PlaneNormal);
		Modifier->MergeMode = MergeMode;
		Modifier->Brush = Brush;
		return ApplyModifier(SculptActor, Modifier);
	}

public:
	/**
	 * Flatten a voxel surface horizontally
	 * @param SculptActor	The sculpt actor to sculpt
	 * @param Center		Center, in world space
	 * @param Normal		Normal of the flattening, usually (0, 0, 1)
	 * @param Radius		Radius, in world space
	 * @param Height		How far up/down to sculpt
	 * @param Falloff		Between 0 and 1
	 * @param Type			Whether to add below, remove above or do both
	 */
	UFUNCTION(BlueprintCallable, Category = "Voxel|Sculpt")
	static FVoxelFuture Flatten(
		UPARAM(Required) AVoxelVolumeSculptActor* SculptActor,
		const FVector Center,
		const FVector Normal = FVector::UpVector,
		const float Radius = 500.f,
		const float Height = 1000.f,
		const float Falloff = 0.1f,
		const EVoxelLevelToolType Type = EVoxelLevelToolType::Additive)
	{
		const TSharedRef<FVoxelFlattenVolumeModifier> Modifier = MakeShared<FVoxelFlattenVolumeModifier>();
		Modifier->Center = Center;
		Modifier->Normal = Normal;
		Modifier->Radius = Radius;
		Modifier->Height = Height;
		Modifier->Falloff = Falloff;
		Modifier->Type = Type;
		return ApplyModifier(SculptActor, Modifier);
	}

public:
	/**
	 * Smooth a voxel sculpt stamp
	 * @param SculptActor	The sculpt actor to sculpt
	 * @param Center		Center, in world space
	 * @param Radius		Radius, in world space
	 * @param Strength		Strength of the smoothing
	 * @param Brush			Optional, used to apply a pattern
	 */
	UFUNCTION(BlueprintCallable, Category = "Voxel|Sculpt")
	static FVoxelFuture Smooth(
		UPARAM(Required) AVoxelVolumeSculptActor* SculptActor,
		const FVector Center,
		const float Radius = 1000.f,
		const float Strength = 1.f,
		const FVoxelToolBrush Brush = FVoxelToolBrush())
	{
		const TSharedRef<FVoxelSmoothVolumeModifier> Modifier = MakeShared<FVoxelSmoothVolumeModifier>();
		Modifier->Center = Center;
		Modifier->Radius = Radius;
		Modifier->Strength = Strength;
		Modifier->Brush = Brush;
		return ApplyModifier(SculptActor, Modifier);
	}

public:
	/**
	 * Add or remove a sphere
	 * @param SculptActor	The sculpt actor to sculpt
	 * @param Center		Center, in world space
	 * @param Radius		Radius, in world space
	 * @param Mode			Whether to add or remove
	 * @param Smoothness	Smoothness of the add/remove
	 * @return
	 */
	UFUNCTION(BlueprintCallable, Category = "Voxel|Sculpt")
	static FVoxelFuture SculptSphere(
		UPARAM(Required) AVoxelVolumeSculptActor* SculptActor,
		const FVector Center,
		const float Radius = 1000.f,
		const EVoxelSculptMode Mode = EVoxelSculptMode::Add,
		const float Smoothness = 0.f)
	{
		const TSharedRef<FVoxelSphereVolumeModifier> Modifier = MakeShared<FVoxelSphereVolumeModifier>();
		Modifier->Center = Center;
		Modifier->Radius = Radius;
		Modifier->Smoothness = Smoothness;
		Modifier->Mode = Mode;
		return ApplyModifier(SculptActor, Modifier);
	}

public:
	/**
	 * Progressively/smoothly sculpt a voxel surface
	 * @param SculptActor	The sculpt actor to sculpt
	 * @param Center		Center, in world space
	 * @param Radius		Radius, in world space
	 * @param Strength		Sculpt speed
	 * @param Mode			Whether to add or remove
	 * @param Brush			Optional, used to apply a pattern
	 */
	UFUNCTION(BlueprintCallable, Category = "Voxel|Sculpt")
	static FVoxelFuture SculptSurface(
		UPARAM(Required) AVoxelVolumeSculptActor* SculptActor,
		const FVector Center,
		const float Radius = 500.f,
		const float Strength = 0.5f,
		const EVoxelSculptMode Mode = EVoxelSculptMode::Add,
		const FVoxelToolBrush Brush = FVoxelToolBrush())
	{
		const TSharedRef<FVoxelSurfaceVolumeModifier> Modifier = MakeShared<FVoxelSurfaceVolumeModifier>();
		Modifier->Center = Center;
		Modifier->Radius = Radius;
		Modifier->Strength = Strength;
		Modifier->Mode = Mode;
		Modifier->Brush = Brush;
		return ApplyModifier(SculptActor, Modifier);
	}

public:
	/**
	 * Apply a sculpt graph
	 * @param SculptActor	The sculpt actor to sculpt
	 * @param Center		Center, in world space
	 * @param Radius		Radius, in world space
	 * @param Graph			Graph
	 * @param Rotation		Rotation to apply to the graph
	 */
	UFUNCTION(BlueprintCallable, Category = "Voxel|Sculpt")
	static FVoxelFuture ApplySculptGraph(
		UPARAM(Required) AVoxelVolumeSculptActor* SculptActor,
		const FVector& Center,
		const float Radius = 500.f,
		const FVoxelVolumeSculptGraphWrapper& Graph = FVoxelVolumeSculptGraphWrapper(),
		const FRotator& Rotation = FRotator::ZeroRotator)
	{
		if (!Graph.IsValid())
		{
			VOXEL_MESSAGE(Error, "Invalid sculpt graph");
			return {};
		}

		const TSharedRef<FVoxelGraphVolumeModifier> Modifier = MakeShared<FVoxelGraphVolumeModifier>();
		Modifier->Transform = FTransform(Rotation, Center);
		Modifier->Radius = Radius;
		Modifier->Graph = Graph;
		return ApplyModifier(SculptActor, Modifier);
	}

public:
	UFUNCTION(BlueprintPure, Category = "Voxel", meta = (NativeMakeFunc))
	static FVoxelVolumeSculptGraphWrapper MakeVoxelVolumeSculptGraphWrapper(UVoxelVolumeSculptGraph* Graph)
	{
		FVoxelVolumeSculptGraphWrapper Result;
		Result.Graph = Graph;
		return Result;
	}
};