// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelNode_MakePoints.h"
#include "Buffer/VoxelFloatBuffers.h"

void FVoxelNode_MakePoints::Compute(const FVoxelGraphQuery Query) const
{
	const TValue<FVoxelTransformBuffer> Transforms = TransformsPin.Get(Query);
	const TValue<FVoxelSeed> Seed = SeedPin.Get(Query);

	VOXEL_GRAPH_WAIT(Transforms, Seed)
	{
		const FVoxelPointRandom Random(Seed, STATIC_HASH("MakePoint"));

		const TSharedRef<FVoxelPointSet> Points = MakeShared<FVoxelPointSet>();
		Points->SetNum(Transforms->Num());

		FVoxelPointIdBuffer Ids;
		Ids.Allocate(Transforms->Num());

		for (int32 Index = 0; Index < Transforms->Num(); Index++)
		{
			Ids.Set(Index, Random.MakeId({}, Index));
		}

		Points->Add(FVoxelPointAttributes::Id, MoveTemp(Ids));
		Points->Add(FVoxelPointAttributes::Position, MakeSharedCopy(Transforms->Translation));
		Points->Add(FVoxelPointAttributes::Rotation, MakeSharedCopy(Transforms->Rotation));
		Points->Add(FVoxelPointAttributes::Scale, MakeSharedCopy(Transforms->Scale));

		OutPin.Set(Query, Points);
	};
}