// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelPinTypeSetRegistry.h"
#include "VoxelNode.h"
#include "VoxelGraph.h"
#include "VoxelTerminalGraph.h"
#include "VoxelObjectPinType.h"
#include "VoxelFunctionLibrary.h"

#include "EdGraph/EdGraphPin.h"
#include "Engine/UserDefinedEnum.h"

#if WITH_EDITOR
FVoxelPinTypeSetRegistry* GVoxelPinTypeSetRegistry = new FVoxelPinTypeSetRegistry();
#endif

#if WITH_EDITOR
const TVoxelSet<FVoxelPinType>& FVoxelPinTypeSetRegistry::GetTypes()
{
	if (Types.Num() == 0)
	{
		InitializeTypes();

		if (VOXEL_DEBUG)
		{
			RunTests();
		}
	}
	check(Types.Num() > 0);

	if (TypesWithUserEnums.Num() == 0)
	{
		InitializeTypesWithUserEnums();
	}
	check(TypesWithUserEnums.Num() > 0);

	return TypesWithUserEnums;
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void FVoxelPinTypeSetRegistry::PreChange(
	const UUserDefinedEnum* Changed,
	FEnumEditorUtils::EEnumEditorChangeInfo ChangedType)
{
}

void FVoxelPinTypeSetRegistry::PostChange(
	const UUserDefinedEnum* Changed,
	FEnumEditorUtils::EEnumEditorChangeInfo ChangedType)
{
	VOXEL_FUNCTION_COUNTER();

	ForEachObjectOfClass<UVoxelGraph>([&](UVoxelGraph& Graph)
	{
		Graph.Fixup();
	});

	ForEachObjectOfClass<UVoxelTerminalGraph>([&](UVoxelTerminalGraph& TerminalGraph)
	{
		TerminalGraph.Fixup();
	});

	// TODO Reconstruct any node with a pin of type Enum

	TypesWithUserEnums.Reset();

	// Refresh enum types
	FVoxelPinTypeSet::OnChanged.Broadcast();
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void FVoxelPinTypeSetRegistry::InitializeTypes()
{
	VOXEL_FUNCTION_COUNTER();
	ensure(Types.Num() == 0);

	TVoxelMap<FVoxelPinType, int32> TypeToPriority;
	TypeToPriority.Reserve(16384);

	ForEachObjectOfClass<UEnum>([&](UEnum& Enum)
	{
		TypeToPriority.FindOrAdd(FVoxelPinType::MakeEnum(&Enum));
	});

	ForEachObjectOfClass<UScriptStruct>([&](UScriptStruct& Struct)
	{
		if (!Struct.HasMetaDataHierarchical(STATIC_FNAME("VoxelPinType")))
		{
			return;
		}

		TypeToPriority.FindOrAdd(FVoxelPinType::MakeStruct(&Struct));
	});

	for (const auto& It : FVoxelObjectPinType::StructToPinType())
	{
		TypeToPriority.FindOrAdd(FVoxelPinType::MakeStruct(ConstCast(It.Key)));
	}

	for (const UScriptStruct* Struct : GetDerivedStructs<FVoxelNode>())
	{
		if (Struct->HasMetaData(STATIC_FNAME("Abstract")) ||
			Struct->HasMetaDataHierarchical(STATIC_FNAME("Internal")))
		{
			continue;
		}

		const TSharedRef<FVoxelNode> Node = MakeSharedStruct<FVoxelNode>(Struct);

		for (const FVoxelPin& Pin : Node->GetPins())
		{
			if (!Pin.IsPromotable())
			{
				TypeToPriority.FindOrAdd(Pin.GetType().GetInnerType())++;
				continue;
			}

			const FVoxelPinTypeSet PromotionTypes = Node->GetPromotionTypes(Pin);
			if (!PromotionTypes.IsExplicit())
			{
				continue;
			}

			for (const FVoxelPinType& Type : PromotionTypes.GetExplicitTypes())
			{
				TypeToPriority.FindOrAdd(Type.GetInnerType())++;
			}
		}
	}
	for (const TSubclassOf<UVoxelFunctionLibrary>& Class : GetDerivedClasses<UVoxelFunctionLibrary>())
	{
		for (const UFunction* Function : GetClassFunctions(Class))
		{
			if (Function->HasMetaData(STATIC_FNAME("Internal")))
			{
				continue;
			}

			for (const FProperty& Property : GetFunctionProperties(Function))
			{
				const FVoxelPinType Type = UVoxelFunctionLibrary::MakeType(Property);
				TypeToPriority.FindOrAdd(Type.GetInnerType())++;
			}
		}
	}

	struct FData
	{
		int32 Priority;
		FString DisplayName;
	};
	TVoxelMap<FVoxelPinType, FData> NewTypeToPriority;
	NewTypeToPriority.Reserve(3 * TypeToPriority.Num());

	for (const auto& It : TypeToPriority)
	{
		ensure(!It.Key.IsBuffer());

		if (It.Key.IsWildcard())
		{
			continue;
		}

		FString DisplayName = It.Key.ToString();
		NewTypeToPriority.Add_EnsureNew(It.Key, FData(It.Value, DisplayName));
		NewTypeToPriority.Add_EnsureNew(It.Key.GetBufferType(), FData(It.Value, DisplayName + " Buffer"));
		NewTypeToPriority.Add_EnsureNew(It.Key.GetBufferType().WithBufferArray(true), FData(It.Value, DisplayName + " Array"));
	}

	NewTypeToPriority.Sort([&](
		const TVoxelMap<FVoxelPinType, FData>::FElement& A,
		const TVoxelMap<FVoxelPinType, FData>::FElement& B)
	{
		if (A.Value.Priority != B.Value.Priority)
		{
			return A.Value.Priority > B.Value.Priority;
		}

		return A.Value.DisplayName < B.Value.DisplayName;
	});

	Types = NewTypeToPriority.KeySet();
}

void FVoxelPinTypeSetRegistry::InitializeTypesWithUserEnums()
{
	VOXEL_FUNCTION_COUNTER();
	ensure(TypesWithUserEnums.Num() == 0);

	TypesWithUserEnums.Append(Types);

	ForEachAssetOfClass<UUserDefinedEnum>([&](UUserDefinedEnum& Enum)
	{
		TypesWithUserEnums.Add(FVoxelPinType::MakeEnum(&Enum));
	});
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void FVoxelPinTypeSetRegistry::RunTests() const
{
	VOXEL_FUNCTION_COUNTER();

	{
		FVoxelPinValue Value(FVoxelPinType::Make<TSubclassOf<UObject>>());
		Value.Get<TSubclassOf<UObject>>() = AActor::StaticClass();
		ensure(Value.Get<TSubclassOf<AActor>>() == AActor::StaticClass());
		ensure(Value.Get<TSubclassOf<AVolume>>() == nullptr);
	}

	for (const FVoxelPinType& Type : Types)
	{
		const FVoxelPinValue ExposedValue(Type.GetExposedType());
		const FVoxelRuntimePinValue RuntimeValue = FVoxelPinType::MakeRuntimeValue(Type, ExposedValue, {});

		const FEdGraphPinType GraphType = Type.GetEdGraphPinType();
		ensure(FVoxelPinType(GraphType) == Type);

		// Array flag is not preserved when going through K2
		// K2 only support doubles
		if (!Type.IsBufferArray() &&
			!Type.GetInnerType().Is<float>())
		{
			const FEdGraphPinType K2Type = Type.GetEdGraphPinType_K2();
			ensure(FVoxelPinType::MakeFromK2(K2Type) == Type);
		}
	}

	TArray<FDateTime> Times;
	Times.Emplace_GetRef() = FDateTime::FromJulianDay(1);
	Times.Emplace_GetRef() = FDateTime::FromJulianDay(2);
	Times.Emplace_GetRef() = FDateTime::FromJulianDay(3);

	TUniquePtr<FStructProperty> StructProperty = FVoxelUtilities::MakeStructProperty(StaticStructFast<FDateTime>());
	const TUniquePtr<FArrayProperty> ArrayProperty = FVoxelUtilities::MakeArrayProperty(StructProperty.Release());

	const FVoxelPinValue Value = FVoxelPinValue::MakeFromProperty(*ArrayProperty, &Times);
	const FVoxelRuntimePinValue RuntimeValue = FVoxelPinType::MakeRuntimeValue(
		FVoxelPinType::Make<FDateTime>().GetBufferType().WithBufferArray(true),
		Value,
		{});

	TArray<FDateTime> NewTimes;
	Value.ExportToProperty(*ArrayProperty, &NewTimes);

	ensure(Times == NewTimes);
}
#endif