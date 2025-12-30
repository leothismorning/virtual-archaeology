// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Volume/VoxelSphereVolumeModifier.h"

FVoxelBox FVoxelSphereVolumeModifier::GetBounds() const
{
	return FVoxelBox(Center - Radius, Center + Radius).Extend(Smoothness * Radius);
}

void FVoxelSphereVolumeModifier::Apply(const FData& Data) const
{
	VOXEL_FUNCTION_COUNTER();

	Data.Apply([&](const float OldDistance, const FVector& Position)
	{
		const double DistanceToCenter = FVector::Distance(Position, Center);

		float NewDistance = DistanceToCenter - Radius;
		if (Mode == EVoxelSculptMode::Remove)
		{
			NewDistance = -NewDistance;
		}

		if (FVoxelUtilities::IsNaN(OldDistance))
		{
			return NewDistance;
		}

		if (Mode == EVoxelSculptMode::Remove)
		{
			return FVoxelUtilities::SmoothMax(OldDistance, NewDistance, Smoothness * Radius);
		}
		else
		{
			checkVoxelSlow(Mode == EVoxelSculptMode::Add);
			return FVoxelUtilities::SmoothMin(OldDistance, NewDistance, Smoothness * Radius);
		}
	});
}