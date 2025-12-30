// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelPointNodes.h"
#include "VoxelPointNodesStats.h"

void FVoxelNode_FilterPoints::Compute(const FVoxelGraphQuery Query) const
{
	const TValue<FVoxelPointSet> Points = InPin.Get(Query);

	VOXEL_GRAPH_WAIT(Points)
	{
		if (Points->Num() == 0)
		{
			return;
		}

		const TValue<FVoxelBoolBuffer> KeepPoint = KeepPointPin.Get(Points->MakeQuery(Query));

		VOXEL_GRAPH_WAIT(Points, KeepPoint)
		{
			if (!Points->CheckNum(this, KeepPoint->Num()))
			{
				OutPin.Set(Query, Points);
				return;
			}

			FindVoxelPointSetAttribute(*Points, FVoxelPointAttributes::Id, FVoxelPointIdBuffer, IdBuffer);
			FVoxelNodeStatScope StatScope(*this, Points->Num());

			FVoxelInt32Buffer Indices;
			Indices.Allocate(Points->Num());

			int32 WriteIndex = 0;
			for (int32 Index = 0; Index < Points->Num(); Index++)
			{
				if ((*KeepPoint)[Index])
				{
					Indices.Set(WriteIndex++, Index);
				}
			}
			Indices.ShrinkTo(WriteIndex);

			FVoxelPointFilterStats::RecordNodeStats(*this, Points->Num(), Indices.Num());

			OutPin.Set(
				Query,
				Points->Gather(Indices.View()));
		};
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNode_DensityFilter::Compute(const FVoxelGraphQuery Query) const
{
	const TValue<FVoxelPointSet> Points = InPin.Get(Query);

	VOXEL_GRAPH_WAIT(Points)
	{
		if (Points->Num() == 0)
		{
			return;
		}

		const TValue<FVoxelFloatBuffer> Densities = DensityPin.Get(Points->MakeQuery(Query));
		const TValue<FVoxelSeed> Seed = SeedPin.Get(Points->MakeQuery(Query));

		VOXEL_GRAPH_WAIT(Points, Densities, Seed)
		{
			if (!Points->CheckNum(this, Densities->Num()))
			{
				OutPin.Set(Query, Points);
				return;
			}

			FindVoxelPointSetAttribute(*Points, FVoxelPointAttributes::Id, FVoxelPointIdBuffer, IdBuffer);
			FVoxelNodeStatScope StatScope(*this, Points->Num());

			const FVoxelPointRandom Random(Seed, STATIC_HASH("DensityFilter"));

			FVoxelInt32Buffer Indices;
			Indices.Allocate(Points->Num());

			int32 WriteIndex = 0;
			for (int32 Index = 0; Index < Points->Num(); Index++)
			{
				const FVoxelPointId Id = IdBuffer[Index];
				const float Fraction = Random.GetFraction(Id);
				if (Fraction >= (*Densities)[Index])
				{
					continue;
				}

				Indices.Set(WriteIndex++, Index);
			}
			Indices.ShrinkTo(WriteIndex);

			FVoxelPointFilterStats::RecordNodeStats(*this, Points->Num(), Indices.Num());

			OutPin.Set(
				Query,
				Points->Gather(Indices.View()));
		};
	};
}