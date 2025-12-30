// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Height/VoxelFlattenHeightModifier.h"

void FVoxelFlattenHeightModifier::Initialize_GameThread()
{
	Super::Initialize_GameThread();

	RuntimeBrush = Brush.GetRuntimeBrush();
}

FVoxelBox2D FVoxelFlattenHeightModifier::GetBounds() const
{
	return FVoxelBox2D(Center - Radius, Center + Radius);
}

void FVoxelFlattenHeightModifier::Apply(const FData& Data) const
{
	VOXEL_FUNCTION_COUNTER();

	Data.Apply([&](const float OldHeight, const FVector2D& Position)
	{
		if (FVoxelUtilities::IsNaN(OldHeight))
		{
			return OldHeight;
		}

		const double DistanceToCenter = FVector2D::Distance(Position, Center);

		const float FalloffMultiplier = RuntimeBrush->GetFalloff(DistanceToCenter, Radius);
		const float MaskStrength = RuntimeBrush->GetMaskStrength(Center, Position, Radius);

		const float NewHeight = FMath::Lerp(OldHeight, TargetHeight, FalloffMultiplier * MaskStrength);

		float Height = OldHeight;

		if (Type == EVoxelLevelToolType::Additive ||
			Type == EVoxelLevelToolType::Both)
		{
			Height = FMath::Max(Height, NewHeight);
		}
		if (Type == EVoxelLevelToolType::Subtractive ||
			Type == EVoxelLevelToolType::Both)
		{
			Height = FMath::Min(Height, NewHeight);
		}

		return Height;
	});
}