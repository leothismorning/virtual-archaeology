// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelPointSet.h"
#include "VoxelNode.h"
#include "VoxelBufferSplitter.h"
#include "Buffer/VoxelDoubleBuffers.h"
#include "VoxelGraphPositionParameter.h"

const FName FVoxelPointAttributes::Id = "Id";
const FName FVoxelPointAttributes::Mesh = "Mesh";
const FName FVoxelPointAttributes::Position = "Position";
const FName FVoxelPointAttributes::Rotation = "Rotation";
const FName FVoxelPointAttributes::Scale = "Scale";
const FName FVoxelPointAttributes::Density = "Density";
const FName FVoxelPointAttributes::BoundsMin = "BoundsMin";
const FName FVoxelPointAttributes::BoundsMax = "BoundsMax";
const FName FVoxelPointAttributes::Color = "Color";
const FName FVoxelPointAttributes::Steepness = "Steepness";

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelPointSet::SetNum(const int32 NewNum)
{
	checkVoxelSlow(NewNum >= 0);
	ensure(PrivateNum == 0);
	ensure(NameToAttribute.Num() == 0);
	PrivateNum = NewNum;
}

void FVoxelPointSet::Add(const FName Name, const TSharedRef<const FVoxelBuffer>& Buffer)
{
	const int32 BufferNum = Buffer->Num_Slow();
	if (!ensure(
		BufferNum == 1 ||
		BufferNum == Num()))
	{
		return;
	}

	NameToAttribute.FindOrAdd(Name) = Buffer;
}

FVoxelGraphQuery FVoxelPointSet::MakeQuery(const FVoxelGraphQuery Query) const
{
	FVoxelGraphQueryImpl& NewQuery = Query->CloneParameters();
	NewQuery.AddParameter<FVoxelGraphParameters::FPointSet>().Value = AsShared();

	if (const TSharedPtr<const FVoxelBuffer> Position = NameToAttribute.FindRef(FVoxelPointAttributes::Position))
	{
		if (const FVoxelVectorBuffer* FloatPosition = Position->As<FVoxelVectorBuffer>())
		{
			NewQuery.AddParameter<FVoxelGraphParameters::FPosition3D>().SetLocalPosition(*FloatPosition);
		}

		if (const FVoxelDoubleVectorBuffer* DoublePosition = Position->As<FVoxelDoubleVectorBuffer>())
		{
			NewQuery.AddParameter<FVoxelGraphParameters::FPosition3D>().SetLocalPosition(*DoublePosition);
		}
	}

	return FVoxelGraphQuery(NewQuery, Query.GetCallstack());
}

bool FVoxelPointSet::CheckNum(const FVoxelNode* Node, int32 BufferNum) const
{
	if (BufferNum != 1 &&
		BufferNum != Num())
	{
		VOXEL_MESSAGE(Error, "{0}: Buffer has Num={1}, but attributes have Num={2}",
			Node,
			BufferNum,
			Num());
		return false;
	}

	return true;
}

TSharedRef<FVoxelPointSet> FVoxelPointSet::Gather(const TConstVoxelArrayView<int32> Indices) const
{
	VOXEL_SCOPE_COUNTER_FORMAT_COND(Num() > 1024, "FVoxelPointSet::Gather Num=%d", Num());

	if (Indices.Num() == 0)
	{
		return MakeShared<FVoxelPointSet>();
	}

	const TSharedRef<FVoxelPointSet> Result = MakeShared<FVoxelPointSet>();
	Result->SetNum(Indices.Num());

	for (const auto& It : NameToAttribute)
	{
		Result->Add(It.Key, It.Value->Gather(Indices));
	}

	return Result;
}

int64 FVoxelPointSet::GetAllocatedSize() const
{
	int64 AllocatedSize = NameToAttribute.GetAllocatedSize();
	for (const auto& It : NameToAttribute)
	{
		AllocatedSize += It.Value->GetAllocatedSize();
	}
	return AllocatedSize;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<const FVoxelPointSet> FVoxelPointSet::Merge(TVoxelArray<TSharedRef<const FVoxelPointSet>> PointSets)
{
	VOXEL_FUNCTION_COUNTER();

	// Not swap to keep order
	PointSets.RemoveAll([&](const TSharedRef<const FVoxelPointSet>& PointSet)
	{
		return PointSet->Num() == 0;
	});

	if (PointSets.Num() == 0)
	{
		return MakeShared<FVoxelPointSet>();
	}
	if (PointSets.Num() == 1)
	{
		return PointSets[0];
	}

	TVoxelSet<FName> AttributeNames;
	for (const TSharedRef<const FVoxelPointSet>& PointSet : PointSets)
	{
		for (const auto& It : PointSet->NameToAttribute)
		{
			AttributeNames.Add(It.Key);
		}
	}

	int32 Num = 0;
	for (const TSharedRef<const FVoxelPointSet>& PointSet : PointSets)
	{
		Num += PointSet->Num();
	}

	const TSharedRef<FVoxelPointSet> Result = MakeShared<FVoxelPointSet>();
	Result->SetNum(Num);

	for (const FName AttributeName : AttributeNames)
	{
		FVoxelPinType InnerType;
		for (const TSharedRef<const FVoxelPointSet>& PointSet : PointSets)
		{
			const TSharedPtr<const FVoxelBuffer> Buffer = PointSet->Find(AttributeName);
			if (!Buffer)
			{
				continue;
			}
			if (!InnerType.IsValid_Fast())
			{
				InnerType = Buffer->GetInnerType();
				continue;
			}

			if (InnerType != Buffer->GetInnerType())
			{
				VOXEL_MESSAGE(Error, "Incompatible point attribute type when merging for {0}: {1} vs {2}",
					AttributeName,
					InnerType.ToString(),
					Buffer->GetInnerType().ToString());
			}
		}

		const TSharedRef<FVoxelBuffer> NewBuffer = FVoxelBuffer::MakeEmpty(InnerType);
		NewBuffer->Allocate(Num);

		int32 WriteIndex = 0;
		for (const TSharedRef<const FVoxelPointSet>& PointSet : PointSets)
		{
			TSharedPtr<const FVoxelBuffer> Buffer = PointSet->Find(AttributeName);
			if (!Buffer ||
				Buffer->GetInnerType() != InnerType)
			{
				if (AttributeName == FVoxelPointAttributes::Rotation &&
					InnerType.Is<FQuat>())
				{
					Buffer = MakeShared<FVoxelQuaternionBuffer>(FQuat4f::Identity);
				}
				else if (
					AttributeName == FVoxelPointAttributes::Scale &&
					InnerType.Is<FVector>())
				{
					Buffer = MakeShared<FVoxelVectorBuffer>(FVector3f::OneVector);
				}
				else
				{
					Buffer = FVoxelBuffer::MakeDefault(InnerType);
				}
			}

			NewBuffer->CopyFrom(*Buffer, 0, WriteIndex, PointSet->Num());
			WriteIndex += PointSet->Num();
		}
		Result->Add(AttributeName, NewBuffer);
	}

	return Result;
}

void FVoxelGraphParameters::FPointSet::Split(
	const FVoxelBufferSplitter& Splitter,
	const TConstVoxelArrayView<FPointSet*> OutResult) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Value->Num());

	TVoxelInlineArray<TSharedPtr<FVoxelPointSet>, 8> PointSets;
	FVoxelUtilities::SetNumZeroed(PointSets, 8);

	for (const int32 Index : Splitter.GetValidOutputs())
	{
		const TSharedRef<FVoxelPointSet> PointSet = MakeShared<FVoxelPointSet>();
		PointSet->SetNum(Splitter.GetOutputNum(Index));
		PointSets[Index] = PointSet;
	}

	for (const TVoxelMapElement<FName, TSharedPtr<const FVoxelBuffer>>& It : Value->GetAttributes())
	{
		TVoxelInlineArray<TSharedPtr<FVoxelBuffer>, 8> BufferRefs;
		FVoxelUtilities::SetNumZeroed(BufferRefs, Splitter.NumOutputs());

		TVoxelInlineArray<FVoxelBuffer*, 8> Buffers;
		FVoxelUtilities::SetNumZeroed(Buffers, Splitter.NumOutputs());

		for (const int32 Index : Splitter.GetValidOutputs())
		{
			const TSharedRef<FVoxelBuffer> Buffer = MakeSharedStruct<FVoxelBuffer>(It.Value->GetStruct());
			BufferRefs[Index] = Buffer;
			Buffers[Index] = &Buffer.Get();
		}

		It.Value->Split(Splitter, Buffers);

		for (const int32 Index : Splitter.GetValidOutputs())
		{
			PointSets[Index]->Add(It.Key, BufferRefs[Index].ToSharedRef());
		}
	}

	for (const int32 Index : Splitter.GetValidOutputs())
	{
		OutResult[Index]->Value = PointSets[Index];
	}
}