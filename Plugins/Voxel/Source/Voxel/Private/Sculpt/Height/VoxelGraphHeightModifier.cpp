// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Height/VoxelGraphHeightModifier.h"
#include "Sculpt/Height/VoxelHeightSculptGraph.h"
#include "Sculpt/VoxelSculptGraphFunctionLibrary.h"
#include "Sculpt/VoxelOutputNode_OutputSculptHeight.h"
#include "VoxelGraphQuery.h"
#include "VoxelGraphContext.h"
#include "VoxelGraphPositionParameter.h"
#include "Buffer/VoxelDoubleBuffers.h"
#include "Utilities/VoxelBufferMathUtilities.h"

void FVoxelGraphHeightModifier::Initialize_GameThread()
{
	if (!Graph.Graph)
	{
		return;
	}

	const TSharedRef<FVoxelGraphEnvironment> Environment = FVoxelGraphEnvironment::Create(
		{},
		{},
		*Graph.Graph,
		Graph.ParameterOverrides,
		FTransform(FVector(Center, 0.f)),
		FVoxelDependencyCollector::Null);

	Evaluator = FVoxelNodeEvaluator::Create<FVoxelOutputNode_OutputSculptHeight>(Environment);
}

FVoxelBox2D FVoxelGraphHeightModifier::GetBounds() const
{
	return FVoxelBox2D(Center - Radius, Center + Radius);
}

void FVoxelGraphHeightModifier::Apply(const FData& Data) const
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelIntBox2D Indices = Data.Indices;

	const float RadiusSquared = FMath::Square(Radius);

	FVoxelInt32Buffer Indirection;
	FVoxelDoubleVector2DBuffer Positions;
	FVoxelFloatBuffer PreviousHeights;
	{
		VOXEL_SCOPE_COUNTER("Write positions");

		Indirection.Allocate(Indices.Count_int32());
		Positions.Allocate(Indices.Count_int32());
		PreviousHeights.Allocate(Indices.Count_int32());

		int32 WriteIndex = 0;
		for (int32 IndexY = Indices.Min.Y; IndexY < Indices.Max.Y; IndexY++)
		{
			for (int32 IndexX = Indices.Min.X; IndexX < Indices.Max.X; IndexX++)
			{
				const FVector2D Position = Data.IndexToWorld.TransformPoint(FVector2D(IndexX, IndexY));
				if (FVector2D::DistSquared(Position, Center) > RadiusSquared)
				{
					continue;
				}

				const int32 ReadIndex = IndexX + Data.Size.X * IndexY;
				const float Height = Data.Heights[ReadIndex];

				Indirection.Set(WriteIndex, ReadIndex);
				Positions.Set(WriteIndex, Position);
				PreviousHeights.Set(WriteIndex, Height);

				WriteIndex++;
			}
		}

		Indirection.ShrinkTo(WriteIndex);
		Positions.ShrinkTo(WriteIndex);
		PreviousHeights.ShrinkTo(WriteIndex);
	}

	FVoxelGraphContext Context = Evaluator.MakeContext(FVoxelDependencyCollector::Null);

	FVoxelGraphQueryImpl& Query = Context.MakeQuery();
	Query.AddParameter<FVoxelGraphParameters::FPosition2D>().SetWorldPosition(Positions);

	{
		FVoxelGraphParameters::FHeightSculpt& Parameter = Query.AddParameter<FVoxelGraphParameters::FHeightSculpt>();
		Parameter.PreviousHeights = PreviousHeights;
		Parameter.IsValid = FVoxelBufferMathUtilities::IsFinite(PreviousHeights);
	}

	const TSharedRef<const FVoxelFloatBuffer> NewHeights = Evaluator->HeightPin.GetSynchronous(Query);

	VOXEL_SCOPE_COUNTER("Write heights");

	for (int32 Index = 0; Index < Indirection.Num(); Index++)
	{
		Data.Heights[Indirection[Index]] = (*NewHeights)[Index];
	}
}