// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Height/VoxelSmoothHeightModifier.h"

void FVoxelSmoothHeightModifier::Initialize_GameThread()
{
	Super::Initialize_GameThread();

	RuntimeBrush = Brush.GetRuntimeBrush();
}

FVoxelBox2D FVoxelSmoothHeightModifier::GetBounds() const
{
	return FVoxelBox2D(Center - Radius, Center + Radius);
}

void FVoxelSmoothHeightModifier::Apply(const FData& Data) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Data.Indices.Count_int32());

	const FIntPoint Size = Data.Size;
	const FVoxelIntBox2D Indices = Data.Indices;

	float CenterStrength = 1;
	float NeighborsStrength = Strength;
	const float Sum = 8 * NeighborsStrength + CenterStrength;
	CenterStrength /= Sum;
	NeighborsStrength /= Sum;

	const TVoxelArray<float> OldHeights = INLINE_LAMBDA
	{
		VOXEL_SCOPE_COUNTER("Copy heights");
		return Data.Heights.Array();
	};

	for (int32 IndexY = Indices.Min.Y; IndexY < Indices.Max.Y; IndexY++)
	{
		for (int32 IndexX = Indices.Min.X; IndexX < Indices.Max.X; IndexX++)
		{
			float& Height = Data.Heights[IndexX + Size.X * IndexY];
			if (FVoxelUtilities::IsNaN(Height))
			{
				continue;
			}

			const auto GetNeighbor = [&](const int32 DX, const int32 DY)
			{
				const int32 NeighborIndexX = IndexX + DX;
				const int32 NeighborIndexY = IndexY + DY;

				const int32 NeighborIndex = NeighborIndexX + Size.X * NeighborIndexY;
				if (!ensure(OldHeights.IsValidIndex(NeighborIndex)))
				{
					return 0.f;
				}

				const float OldHeight = OldHeights[NeighborIndex];
				if (FVoxelUtilities::IsNaN(OldHeight))
				{
					return 0.f;
				}

				return OldHeight * NeighborsStrength;
			};

			const float NeighborSum =
				GetNeighbor(-1, -1) +
				GetNeighbor(+0, -1) +
				GetNeighbor(+1, -1) +
				GetNeighbor(-1, +0) +
				GetNeighbor(+1, +0) +
				GetNeighbor(-1, +1) +
				GetNeighbor(+0, +1) +
				GetNeighbor(+1, +1);

			const FVector2D Position = Data.IndexToWorld.TransformPoint(FVector2D(IndexX, IndexY));
			const double DistanceToCenter = FVector2D::Distance(Position, Center);

			const float FalloffMultiplier = RuntimeBrush->GetFalloff(DistanceToCenter, Radius);
			const float MaskStrength = RuntimeBrush->GetMaskStrength(Center, Position, Radius);

			Height = FMath::Lerp(Height, NeighborSum + Height * CenterStrength, FalloffMultiplier * MaskStrength);
		}
	}
}