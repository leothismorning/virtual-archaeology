// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Volume/VoxelAngleVolumeModifier.h"

void FVoxelAngleVolumeModifier::Initialize_GameThread()
{
	Super::Initialize_GameThread();

	RuntimeBrush = Brush.GetRuntimeBrush();
}

FVoxelBox FVoxelAngleVolumeModifier::GetBounds() const
{
	return FVoxelBox(Center - Radius, Center + Radius);
}

void FVoxelAngleVolumeModifier::Apply(const FData& Data) const
{
	VOXEL_FUNCTION_COUNTER();

	Data.Apply([&](const float OldDistance, const FVector& Position)
	{
		if (FVoxelUtilities::IsNaN(OldDistance))
		{
			return OldDistance;
		}

		const double DistanceToCenter = FVector::Distance(Position, Center);

		if (DistanceToCenter > Radius)
		{
			return OldDistance;
		}

		float Alpha = RuntimeBrush->GetFalloff(DistanceToCenter, Radius);
		Alpha *= Strength;
		Alpha *= RuntimeBrush->GetMaskStrength(Center, Position, Radius);

		const float DistanceToPlane = Plane.PlaneDot(Position);

		const float NewDistance = INLINE_LAMBDA
		{
			switch (MergeMode)
			{
			default: ensure(false);
			case EVoxelSDFMergeMode::Union: return FMath::Min(OldDistance, DistanceToPlane);
			case EVoxelSDFMergeMode::Intersection: return FMath::Max(DistanceToPlane, OldDistance);
			case EVoxelSDFMergeMode::Override: return DistanceToPlane;
			}
		};

		return FMath::Lerp(OldDistance, NewDistance, FMath::Clamp(Alpha, 0.f, 1.f));
	});
}