// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Volume/VoxelSmoothVolumeModifier.h"

void FVoxelSmoothVolumeModifier::Initialize_GameThread()
{
	Super::Initialize_GameThread();

	RuntimeBrush = Brush.GetRuntimeBrush();
}

FVoxelBox FVoxelSmoothVolumeModifier::GetBounds() const
{
	return FVoxelBox(Center - Radius, Center + Radius);
}

void FVoxelSmoothVolumeModifier::Apply(const FData& Data) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Data.Indices.Count_int32());

	const FIntVector Size = Data.Size;
	const FVoxelIntBox Indices = Data.Indices;

	float CenterStrength = 1;
	float NeighborsStrength = Strength;
	const float Sum = 26 * NeighborsStrength + CenterStrength;
	CenterStrength /= Sum;
	NeighborsStrength /= Sum;

	const TVoxelArray<float> OldDistances = INLINE_LAMBDA
	{
		VOXEL_SCOPE_COUNTER("Copy distances");
		return Data.Distances.Array();
	};

	Voxel::ParallelFor(Indices.GetZ(), [&](const int32 IndexZ)
	{
		for (int32 IndexY = Indices.Min.Y; IndexY < Indices.Max.Y; IndexY++)
		{
			for (int32 IndexX = Indices.Min.X; IndexX < Indices.Max.X; IndexX++)
			{
				float& Distance = Data.Distances[IndexX + Size.X * IndexY + Size.X * Size.Y * IndexZ];
				if (FVoxelUtilities::IsNaN(Distance))
				{
					continue;
				}

				const auto GetNeighbor = [&](const int32 DX, const int32 DY, const int32 DZ)
				{
					const int32 NeighborIndexX = IndexX + DX;
					const int32 NeighborIndexY = IndexY + DY;
					const int32 NeighborIndexZ = IndexZ + DZ;

					const int32 NeighborIndex = NeighborIndexX + Size.X * NeighborIndexY + Size.X * Size.Y * NeighborIndexZ;
					if (!ensure(OldDistances.IsValidIndex(NeighborIndex)))
					{
						return 0.f;
					}

					const float OldDistance = OldDistances[NeighborIndex];
					if (FVoxelUtilities::IsNaN(OldDistance))
					{
						return 0.f;
					}

					return OldDistance * NeighborsStrength;
				};

				const float NeighborSum =
					GetNeighbor(-1, -1, -1) +
					GetNeighbor(+0, -1, -1) +
					GetNeighbor(+1, -1, -1) +
					GetNeighbor(-1, +0, -1) +
					GetNeighbor(+0, +0, -1) +
					GetNeighbor(+1, +0, -1) +
					GetNeighbor(-1, +1, -1) +
					GetNeighbor(+0, +1, -1) +
					GetNeighbor(+1, +1, -1) +
					GetNeighbor(-1, -1, +0) +
					GetNeighbor(+0, -1, +0) +
					GetNeighbor(+1, -1, +0) +
					GetNeighbor(-1, +0, +0) +
					GetNeighbor(+1, +0, +0) +
					GetNeighbor(-1, +1, +0) +
					GetNeighbor(+0, +1, +0) +
					GetNeighbor(+1, +1, +0) +
					GetNeighbor(-1, -1, +1) +
					GetNeighbor(+0, -1, +1) +
					GetNeighbor(+1, -1, +1) +
					GetNeighbor(-1, +0, +1) +
					GetNeighbor(+0, +0, +1) +
					GetNeighbor(+1, +0, +1) +
					GetNeighbor(-1, +1, +1) +
					GetNeighbor(+0, +1, +1) +
					GetNeighbor(+1, +1, +1);

				const FVector Position = Data.IndexToWorld.TransformPosition(FVector(IndexX, IndexY, IndexZ));
				const double DistanceToCenter = FVector::Distance(Position, Center);

				const float FalloffMultiplier = RuntimeBrush->GetFalloff(DistanceToCenter, Radius);
				const float MaskStrength = RuntimeBrush->GetMaskStrength(Center, Position, Radius);

				Distance = FMath::Lerp(Distance, NeighborSum + Distance * CenterStrength, FalloffMultiplier * MaskStrength);
			}
		}
	});
}