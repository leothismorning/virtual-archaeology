// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Volume/VoxelSurfaceVolumeModifier.h"

void FVoxelSurfaceVolumeModifier::Initialize_GameThread()
{
	Super::Initialize_GameThread();

	RuntimeBrush = Brush.GetRuntimeBrush();
}

FVoxelBox FVoxelSurfaceVolumeModifier::GetBounds() const
{
	return FVoxelBox(Center - Radius, Center + Radius);
}

void FVoxelSurfaceVolumeModifier::Apply(const FData& Data) const
{
	VOXEL_FUNCTION_COUNTER();

	Data.Apply([&](const float OldDistance, const FVector& Position)
	{
		if (FVoxelUtilities::IsNaN(OldDistance))
		{
			return OldDistance;
		}

		const double DistanceToCenter = FVector::Distance(Position, Center);

		const float FalloffMultiplier = RuntimeBrush->GetFalloff(DistanceToCenter, Radius);
		const float MaskStrength = RuntimeBrush->GetMaskStrength(Center, Position, Radius);
		const float Delta = Radius / 10 * FalloffMultiplier * Strength * MaskStrength;

		if (Mode == EVoxelSculptMode::Add)
		{
			return OldDistance - Delta;
		}
		else
		{
			checkVoxelSlow(Mode == EVoxelSculptMode::Remove);
			return OldDistance + Delta;
		}
	});
}