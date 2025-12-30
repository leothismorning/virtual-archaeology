// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Volume/VoxelGraphVolumeModifier.h"
#include "Sculpt/Volume/VoxelVolumeSculptGraph.h"
#include "Sculpt/VoxelSculptGraphFunctionLibrary.h"
#include "Sculpt/VoxelOutputNode_OutputSculptDistance.h"
#include "VoxelGraphQuery.h"
#include "VoxelGraphContext.h"
#include "VoxelGraphPositionParameter.h"
#include "Buffer/VoxelDoubleBuffers.h"

void FVoxelGraphVolumeModifier::Initialize_GameThread()
{
	Super::Initialize_GameThread();

	if (!Graph.Graph)
	{
		return;
	}

	const TSharedRef<FVoxelGraphEnvironment> Environment = FVoxelGraphEnvironment::Create(
		{},
		{},
		*Graph.Graph,
		Graph.ParameterOverrides,
		Transform,
		FVoxelDependencyCollector::Null);

	Evaluator = FVoxelNodeEvaluator::Create<FVoxelOutputNode_OutputSculptDistance>(Environment);
}

FVoxelBox FVoxelGraphVolumeModifier::GetBounds() const
{
	return FVoxelBox(Transform.GetLocation() - Radius, Transform.GetLocation() + Radius);
}

void FVoxelGraphVolumeModifier::Apply(const FData& Data) const
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelIntBox Indices = Data.Indices;
	const float RadiusSquared = FMath::Square(Radius);
	const FVector Center = Transform.GetLocation();

	FVoxelInt32Buffer Indirection;
	FVoxelDoubleVectorBuffer Positions;
	FVoxelFloatBuffer PreviousDistances;
	{
		VOXEL_SCOPE_COUNTER("Write positions");

		Indirection.Allocate(Indices.Count_int32());
		Positions.Allocate(Indices.Count_int32());
		PreviousDistances.Allocate(Indices.Count_int32());

		int32 WriteIndex = 0;
		for (int32 IndexZ = Indices.Min.Z; IndexZ < Indices.Max.Z; IndexZ++)
		{
			for (int32 IndexY = Indices.Min.Y; IndexY < Indices.Max.Y; IndexY++)
			{
				for (int32 IndexX = Indices.Min.X; IndexX < Indices.Max.X; IndexX++)
				{
					const FVector Position = Data.IndexToWorld.TransformPosition(FVector(IndexX, IndexY, IndexZ));
					if (FVector::DistSquared(Position, Center) > RadiusSquared)
					{
						continue;
					}

					const int32 ReadIndex = IndexX + Data.Size.X * IndexY + Data.Size.X * Data.Size.Y * IndexZ;
					const float Distance = Data.Distances[ReadIndex];

					Indirection.Set(WriteIndex, ReadIndex);
					Positions.Set(WriteIndex, Position);
					PreviousDistances.Set(WriteIndex, Distance);

					WriteIndex++;
				}
		}
		}

		Indirection.ShrinkTo(WriteIndex);
		Positions.ShrinkTo(WriteIndex);
		PreviousDistances.ShrinkTo(WriteIndex);
	}

	FVoxelGraphContext Context = Evaluator.MakeContext(FVoxelDependencyCollector::Null);

	FVoxelGraphQueryImpl& Query = Context.MakeQuery();
	Query.AddParameter<FVoxelGraphParameters::FPosition3D>().SetWorldPosition(Positions);

	{
		FVoxelGraphParameters::FVolumeSculpt& Parameter = Query.AddParameter<FVoxelGraphParameters::FVolumeSculpt>();
		Parameter.PreviousDistances = PreviousDistances;
	}

	const TSharedRef<const FVoxelFloatBuffer> NewDistances = Evaluator->DistancePin.GetSynchronous(Query);

	VOXEL_SCOPE_COUNTER("Write Distances");

	for (int32 Index = 0; Index < Indirection.Num(); Index++)
	{
		Data.Distances[Indirection[Index]] = (*NewDistances)[Index];
	}
}