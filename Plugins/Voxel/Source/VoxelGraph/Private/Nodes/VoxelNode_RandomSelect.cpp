// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nodes/VoxelNode_RandomSelect.h"

void FVoxelNode_RandomSelect::Compute(const FVoxelGraphQuery Query) const
{
	const TValue<FVoxelSeedBuffer> Seeds = SeedPin.Get(Query);
	const TValue<FVoxelBuffer> Values = ValuesPin.Get<FVoxelBuffer>(Query);
	const TValue<FVoxelFloatBuffer> Weights = WeightsPin.Get(Query);

	VOXEL_GRAPH_WAIT(Seeds, Values, Weights)
	{
		const int32 NumValues = Values->Num_Slow();
		if (NumValues == 0)
		{
			// Make sure to return an empty buffer
			ResultPin.Set(Query, FVoxelRuntimePinValue::Make(Values));
			IndexPin.Set(Query, FVoxelInt32Buffer());
			return;
		}

		FVoxelNodeStatScope StatScope(*this, Seeds->Num());

		FVoxelInt32Buffer Indices;
		Indices.Allocate(Seeds->Num());

		const int32 Random = STATIC_HASH("RandomSelect");

		if (Weights->Num() > 0)
		{
			if (Weights->Num() != NumValues)
			{
				VOXEL_MESSAGE(Error, "Values.Num={0} but Weights.Num={1}", NumValues, Weights->Num());
				return;
			}

			TVoxelArray<double> RunningSum;
			FVoxelUtilities::SetNumFast(RunningSum, NumValues);

			double TotalSum = 0.;
			for (int32 Index = 0; Index < NumValues; Index++)
			{
				const float Weight = FMath::Max((*Weights)[Index], 0.f);
				TotalSum += Weight;

				if (Index == 0)
				{
					RunningSum[Index] = Weight;
				}
				else
				{
					RunningSum[Index] = RunningSum[Index - 1] + Weight;
				}
			}

			for (int32 Index = 0; Index < Seeds->Num(); Index++)
			{
				const FVoxelSeed Seed = (*Seeds)[Index];
				const uint32 LocalSeed = FVoxelUtilities::MurmurHash(uint32(Seed), Random);
				const double Value = TotalSum * FVoxelUtilities::GetFraction(LocalSeed);

				for (int32 ValueIndex = 0; ValueIndex < NumValues; ValueIndex++)
				{
					if (Value <= RunningSum[ValueIndex])
					{
						Indices.Set(Index, ValueIndex);
						goto End;
					}
				}

				ensureVoxelSlow(false);
				Indices.Set(Index, 0);

			End:
				;
			}
		}
		else
		{
			for (int32 Index = 0; Index < Seeds->Num(); Index++)
			{
				const FVoxelSeed Seed = (*Seeds)[Index];
				const uint32 LocalSeed = FVoxelUtilities::MurmurHash(uint32(Seed), Random);
				Indices.Set(Index, FVoxelUtilities::RandRange(LocalSeed, 0, NumValues - 1));
			}
		}

		const TSharedRef<const FVoxelBuffer> Buffer = Values->Gather(Indices.View());

		ResultPin.Set(Query, FVoxelRuntimePinValue::Make(Buffer));
		IndexPin.Set(Query, MoveTemp(Indices));
	};
}

#if WITH_EDITOR
FVoxelPinTypeSet FVoxelNode_RandomSelect::GetPromotionTypes(const FVoxelPin& Pin) const
{
	if (Pin == ValuesPin)
	{
		return FVoxelPinTypeSet::AllBufferArrays();
	}
	else
	{
		return FVoxelPinTypeSet::AllBuffers();
	}
}

void FVoxelNode_RandomSelect::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	Pin.SetType(NewType);

	if (Pin == ValuesPin)
	{
		GetPin(ResultPin).SetType(NewType.WithBufferArray(false));
	}
	else
	{
		GetPin(ValuesPin).SetType(NewType.WithBufferArray(true));
	}
}
#endif