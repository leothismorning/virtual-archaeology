// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Volume/VoxelFlattenVolumeModifier.h"

FVoxelBox FVoxelFlattenVolumeModifier::GetBounds() const
{
	return FVoxelBox(Center - Radius, Center + Radius);
}

void FVoxelFlattenVolumeModifier::Apply(const FData& Data) const
{
	VOXEL_FUNCTION_COUNTER();

	const float InternalRadius = Radius * (1.f - Falloff);
	const float ExternalRadius = Radius * Falloff;

	const FQuat PlaneDirection = FRotationMatrix::MakeFromZ(Normal).ToQuat();
	const FVector Offset = PlaneDirection.RotateVector(FVector(0.f, 0.f, Height / 2.f));
	const FQuat InvertedPlaneDirection = PlaneDirection.Inverse();

	Data.Apply([&](const float OldDistance, const FVector& Position)
	{
		if (FVoxelUtilities::IsNaN(OldDistance))
		{
			return OldDistance;
		}

		const auto GetNewDistance = [&](const float Direction) -> float
		{
			FVector PositionRelativeToCenter = Position - (Center + Offset * Direction);
			PositionRelativeToCenter = InvertedPlaneDirection.RotateVector(PositionRelativeToCenter);

			const float DistanceToCenterXY = FVector2D(PositionRelativeToCenter.X, PositionRelativeToCenter.Y).Size();
			const float DistanceToCenterZ = FMath::Abs(PositionRelativeToCenter.Z);

			const float SidesDistance = DistanceToCenterXY - InternalRadius;
			const float TopDistance = DistanceToCenterZ - Height / 2 + ExternalRadius;

			return
				FMath::Min(FMath::Max(SidesDistance, TopDistance), 0.0f) +
				FVector2D(FMath::Max(SidesDistance, 0.f), FMath::Max(TopDistance, 0.f)).Size() +
				-ExternalRadius;
		};

		switch (Type)
		{
		default: ensure(false);
		case EVoxelLevelToolType::Additive: return FMath::Min(OldDistance, GetNewDistance(-1.f));
		case EVoxelLevelToolType::Subtractive: return FMath::Max(OldDistance, -GetNewDistance(+1.f));
		case EVoxelLevelToolType::Both: return FMath::Max(FMath::Min(OldDistance, GetNewDistance(-1.f)),- GetNewDistance(+1.f));
		}
	});
}