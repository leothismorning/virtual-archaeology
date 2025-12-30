// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelPointAttributeNodes.h"
#include "VoxelBufferAccessor.h"
#include "VoxelPointUtilities.h"
#include "VoxelCompilationGraph.h"
#include "Nodes/VoxelVectorNodes.h"
#include "Buffer/VoxelFloatBuffers.h"
#include "Buffer/VoxelIntegerBuffers.h"
#include "Utilities/VoxelBufferMathUtilities.h"
#include "Metadata/Accessors/PCGAttributeExtractor.h"

void FVoxelNode_SetPointAttribute::Compute(const FVoxelGraphQuery Query) const
{
	const TValue<FVoxelPointSet> Points = InPin.Get(Query);

	VOXEL_GRAPH_WAIT(Points)
	{
		const TValue<FName> BaseName = NamePin.Get(Query);
		const TValue<FVoxelBuffer> Value = ValuePin.Get<FVoxelBuffer>(Points->MakeQuery(Query));

		VOXEL_GRAPH_WAIT(Points, BaseName, Value)
		{
			if (!Points->CheckNum(this, Value->Num_Slow()))
			{
				OutPin.Set(Query, Points);
				return;
			}

			const FName Name = AddParentPrefix(BaseName);

			FName AttributeName;
			TVoxelArray<FString> Extractors;
			const TSharedPtr<const FVoxelBuffer> ExistingAttribute = FVoxelPointUtilities::FindExtractedAttribute(
				*Points,
				Name,
				AttributeName,
				Extractors);

			if (Extractors.Num() == 0)
			{
				const TSharedRef<FVoxelPointSet> NewPoints = Points->MakeSharedCopy();
				NewPoints->Add(Name, Value);
				OutPin.Set(Query, NewPoints);
				return;
			}

			if (!ExistingAttribute)
			{
				VOXEL_MESSAGE(Error, "{0}: Attribute {1} does not exist",
					this,
					AttributeName);

				OutPin.Set(Query, Points);
				return;
			}

			if (!ensure(ExistingAttribute->IsA<FVoxelBufferStruct>()))
			{
				VOXEL_MESSAGE(Error, "{0}: Invalid attribute {1}, should be a ",
					this,
					AttributeName);

				OutPin.Set(Query, Points);
				return;
			}

			const TSharedRef<FVoxelPointSet> NewPoints = Points->MakeSharedCopy();
			const TSharedRef<FVoxelBufferStruct> NewAttribute = MakeSharedCopy(ExistingAttribute->AsChecked<FVoxelBufferStruct>());

			FVoxelPointUtilities::SetExtractedAttribute(
				*NewAttribute,
				Value,
				Extractors,
				0,
				AttributeName,
				GetNodeRef());

			NewPoints->Add(AttributeName, NewAttribute);

			OutPin.Set(Query, NewPoints);
		};
	};
}

#if WITH_EDITOR
FVoxelPinTypeSet FVoxelNode_SetPointAttribute::GetPromotionTypes(const FVoxelPin& Pin) const
{
	return FVoxelPinTypeSet::AllBuffers();
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNode_GetPointAttribute::Compute(const FVoxelGraphQuery Query) const
{
	const TValue<FName> BaseName = NamePin.Get(Query);

	VOXEL_GRAPH_WAIT(BaseName)
	{
		const FVoxelGraphParameters::FPointSet* Parameter = Query->FindParameter<FVoxelGraphParameters::FPointSet>();
		if (!Parameter)
		{
			VOXEL_MESSAGE(Error, "{0}: No points passed in input", this);
			return;
		}

		const TSharedRef<const FVoxelPointSet> PointSet = Parameter->Value.ToSharedRef();
		if (PointSet->Num() == 0)
		{
			return;
		}

		const FName Name = AddParentPrefix(BaseName);
		const FVoxelPinType ReturnType = ValuePin.GetType_RuntimeOnly();

		// If the full attribute name exists, use it
		if (const TSharedPtr<const FVoxelBuffer> Attribute = PointSet->Find(Name))
		{
			if (!Attribute->GetBufferType().CanBeCastedTo(ReturnType))
			{
				VOXEL_MESSAGE(Error, "{0}: Found attribute named {1} of type {2}, cannot cast to {3}",
					this,
					Name,
					Attribute->GetBufferType().ToString(),
					ReturnType.ToString());

				return;
			}

			ValuePin.Set(Query, FVoxelRuntimePinValue::Make(Attribute.ToSharedRef()));
			return;
		}

		FName AttributeName;
		TVoxelArray<FString> Extractors;
		const TSharedPtr<const FVoxelBuffer> Attribute = FVoxelPointUtilities::FindExtractedAttribute(
			*PointSet,
			Name,
			AttributeName,
			Extractors);

		if (!Attribute)
		{
			VOXEL_MESSAGE(Error, "{0}: No attribute named {1} found", this, Name);
			return;
		}

		const TSharedPtr<const FVoxelBuffer> Buffer = ExtractData(
			Attribute.ToSharedRef(),
			Extractors,
			0,
			AttributeName,
			Query);

		if (!ensure(Buffer))
		{
			VOXEL_MESSAGE(Error, "{0}: Failed to extract {1}", this, AttributeName);
			return;
		}

		ValuePin.Set(Query, FVoxelRuntimePinValue::Make(Buffer.ToSharedRef()));
	};
}

#if WITH_EDITOR
FVoxelPinTypeSet FVoxelNode_GetPointAttribute::GetPromotionTypes(const FVoxelPin& Pin) const
{
	return FVoxelPinTypeSet::AllBuffers();
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<const FVoxelBuffer> FVoxelNode_GetPointAttribute::ExtractData(
	const TSharedRef<const FVoxelBuffer>& Buffer,
	const TConstVoxelArrayView<FString> Extractors,
	const int32 ExtractorIndex,
	const FName AttributeName,
	const FVoxelGraphQuery Query) const
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelPinType ReturnType = ValuePin.GetType_RuntimeOnly();
	const FString& Extractor = Extractors[ExtractorIndex];

	const TSharedPtr<const FVoxelBuffer> NewValue = INLINE_LAMBDA -> TSharedPtr<const FVoxelBuffer>
	{
#define CALL_EXTRACTOR(Type, ExtractorFunc) \
		if (Buffer->GetStruct() == StaticStructFast<Type>()) \
		{ \
			return ExtractorFunc( \
				StaticCastSharedRef<const Type>(Buffer), \
				Extractor, \
				ExtractorIndex + 1 >= Extractors.Num() ? ReturnType : FVoxelPinType(), \
				AttributeName, \
				GetNodeRef()); \
		}

		CALL_EXTRACTOR(FVoxelLinearColorBuffer, FVoxelPointUtilities::ExtractVector);
		CALL_EXTRACTOR(FVoxelVectorBuffer, FVoxelPointUtilities::ExtractVector);
		CALL_EXTRACTOR(FVoxelVector2DBuffer, FVoxelPointUtilities::ExtractVector);
		CALL_EXTRACTOR(FVoxelIntVector4Buffer, FVoxelPointUtilities::ExtractVector);
		CALL_EXTRACTOR(FVoxelIntVectorBuffer, FVoxelPointUtilities::ExtractVector);
		CALL_EXTRACTOR(FVoxelIntPointBuffer, FVoxelPointUtilities::ExtractVector);
		CALL_EXTRACTOR(FVoxelQuaternionBuffer, FVoxelPointUtilities::ExtractQuat);
		CALL_EXTRACTOR(FVoxelTransformBuffer, FVoxelPointUtilities::ExtractTransform);

#undef CALL_EXTRACTOR

		VOXEL_MESSAGE(Error, "{0}: Cannot extract data from attribute {1} with type {2}",
			this,
			AttributeName,
			ReturnType.ToString());

		return {};
	};

	if (!NewValue)
	{
		return {};
	}

	ensure(ExtractorIndex < Extractors.Num());

	if (ExtractorIndex + 1 >= Extractors.Num())
	{
		return NewValue;
	}

	return ExtractData(
		NewValue.ToSharedRef(),
		Extractors,
		ExtractorIndex + 1,
		AttributeName,
		Query);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelTemplateNodeUtilities::FPin* FVoxelTemplateNode_SetPointAttributeBase::ExpandPins(
	FNode& Node,
	TArray<FPin*> Pins,
	const TArray<FPin*>& AllPins) const
{
	check(Pins.Num() == 2);
	check(AllPins.Num() == 3);

	Pins.Insert(MakeConstant(Node, FVoxelPinValue::Make(GetAttributeName())), 1);

	return Call_Single<FVoxelNode_SetPointAttribute>(Pins);
}

FVoxelTemplateNodeUtilities::FPin* FVoxelTemplateNode_GetPointAttributeBase::ExpandPins(
	FNode& Node,
	TArray<FPin*> Pins,
	const TArray<FPin*>& AllPins) const
{
	check(Pins.Num() == 0);
	check(AllPins.Num() == 1);

	Pins.Add(MakeConstant(Node, FVoxelPinValue::Make(GetAttributeName())));

	return Call_Single<FVoxelNode_GetPointAttribute>(Pins, AllPins[0]->Type);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNode_GetPointSeed::Compute(const FVoxelGraphQuery Query) const
{
	const TValue<FVoxelSeed> Seed = SeedPin.Get(Query);

	VOXEL_GRAPH_WAIT(Seed)
	{
		const FVoxelGraphParameters::FPointSet* Parameter = Query->FindParameter<FVoxelGraphParameters::FPointSet>();
		if (!Parameter)
		{
			VOXEL_MESSAGE(Error, "{0}: No points passed in input", this);
			return;
		}

		FindVoxelPointSetAttribute(*Parameter->Value, FVoxelPointAttributes::Id, FVoxelPointIdBuffer, Buffer);

		ValuePin.Set(Query, FVoxelPointUtilities::PointIdToSeed(Buffer, Seed));
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNode_ApplyTranslation::Compute(const FVoxelGraphQuery Query) const
{
	const TValue<FVoxelPointSet> Points = InPin.Get(Query);

	VOXEL_GRAPH_WAIT(Points)
	{
		const TValue<FVoxelVectorBuffer> Translation = TranslationPin.Get(Points->MakeQuery(Query));

		VOXEL_GRAPH_WAIT(Points, Translation)
		{
			CheckVoxelBuffersNum(*Points, Translation);
			FindVoxelPointSetAttribute(*Points, FVoxelPointAttributes::Position, FVoxelVectorBuffer, PositionBuffer);

			const TSharedRef<FVoxelPointSet> NewPoints = Points->MakeSharedCopy();
			NewPoints->Add(FVoxelPointAttributes::Position, FVoxelBufferMathUtilities::Add(PositionBuffer, *Translation));
			OutPin.Set(Query, NewPoints);
		};
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNode_ApplyRotation::Compute(const FVoxelGraphQuery Query) const
{
	const TValue<FVoxelPointSet> Points = InPin.Get(Query);

	VOXEL_GRAPH_WAIT(Points)
	{
		const TValue<FVoxelQuaternionBuffer> Rotation = RotationPin.Get(Points->MakeQuery(Query));

		VOXEL_GRAPH_WAIT(Points, Rotation)
		{
			CheckVoxelBuffersNum(*Points, Rotation);
			FindVoxelPointSetAttribute(*Points, FVoxelPointAttributes::Rotation, FVoxelQuaternionBuffer, RotationBuffer);

			const TSharedRef<FVoxelPointSet> NewPoints = Points->MakeSharedCopy();
			NewPoints->Add(FVoxelPointAttributes::Rotation, FVoxelBufferMathUtilities::Combine(RotationBuffer, *Rotation));
			OutPin.Set(Query, NewPoints);
		};
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNode_ApplyScale::Compute(const FVoxelGraphQuery Query) const
{
	const TValue<FVoxelPointSet> Points = InPin.Get(Query);

	VOXEL_GRAPH_WAIT(Points)
	{
		const TValue<FVoxelVectorBuffer> Scale = ScalePin.Get(Points->MakeQuery(Query));

		VOXEL_GRAPH_WAIT(Points, Scale)
		{
			CheckVoxelBuffersNum(*Points, Scale);
			FindVoxelPointSetAttribute(*Points, FVoxelPointAttributes::Scale, FVoxelVectorBuffer, ScaleBuffer);

			const TSharedRef<FVoxelPointSet> NewPoints = Points->MakeSharedCopy();
			NewPoints->Add(FVoxelPointAttributes::Scale, FVoxelBufferMathUtilities::Multiply(ScaleBuffer, *Scale));
			OutPin.Set(Query, NewPoints);
		};
	};
}