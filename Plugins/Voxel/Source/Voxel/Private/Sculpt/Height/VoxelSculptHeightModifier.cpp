// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Height/VoxelSculptHeightModifier.h"

void FVoxelSculptHeightModifier::Initialize_GameThread()
{
	Super::Initialize_GameThread();

	RuntimeBrush = Brush.GetRuntimeBrush();
}

FVoxelBox2D FVoxelSculptHeightModifier::GetBounds() const
{
	return FVoxelBox2D(Center - Radius, Center + Radius);
}

void FVoxelSculptHeightModifier::Apply(const FData& Data) const
{
	VOXEL_FUNCTION_COUNTER();

	Data.Apply([&](const float OldHeight, const FVector2D& Position)
	{
		const double Distance = FVector2D::Distance(Position, Center);
		const float FalloffMultiplier = RuntimeBrush->GetFalloff(Distance, Radius);
		const float MaskStrength = RuntimeBrush->GetMaskStrength(Center, Position, Radius);
		const float Delta = Radius / 10 * FalloffMultiplier * Strength * MaskStrength;

		if (Mode == EVoxelSculptMode::Add)
		{
			return OldHeight + Delta;
		}
		else
		{
			checkVoxelSlow(Mode == EVoxelSculptMode::Remove);
			return OldHeight - Delta;
		}
	});
}