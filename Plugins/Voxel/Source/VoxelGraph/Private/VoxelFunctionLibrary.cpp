// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelFunctionLibrary.h"
#include "VoxelBuffer.h"
#include "VoxelGenericStructBuffer.h"
#include "VoxelNodeStats.h"
#include "Nodes/VoxelNode_UFunction.h"

void UVoxelFunctionLibrary::RaiseBufferError() const
{
	PrivateNode->RaiseBufferError();
}

TSharedRef<FVoxelMessageToken> UVoxelFunctionLibrary::CreateMessageToken() const
{
	return PrivateNode->GetNodeRef().CreateMessageToken();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelPinType UVoxelFunctionLibrary::MakeType(const FProperty& Property)
{
	if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		if (StructProperty->Struct == StaticStructFast<FVoxelRuntimePinValue>())
		{
			return FVoxelPinType::MakeWildcard();
		}
		if (StructProperty->Struct->IsChildOf(StaticStructFast<FVoxelBuffer>()))
		{
			return FVoxelBuffer::FindInnerType(StructProperty->Struct).GetBufferType();
		}
	}

	return FVoxelPinType(Property);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UVoxelFunctionLibrary::FCachedFunction::FCachedFunction(const UFunction& Function)
	: Function(Function)
{
#if PLATFORM_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type"
#endif
	NativeFunc = reinterpret_cast<FNativeFuncPtr>(Function.GetNativeFunc());
#if PLATFORM_COMPILER_CLANG
#pragma clang diagnostic pop
#endif

	ReturnProperty = Function.GetReturnProperty();

	// ReturnProperty should be the last property
	ensure(!ReturnProperty || !ReturnProperty->PropertyLinkNext);

	if (ReturnProperty)
	{
		ReturnPropertyType = MakeType(*ReturnProperty);

		if (ReturnPropertyType.IsWildcard())
		{
			Struct = StaticStructFast<FVoxelRuntimePinValue>();
		}
		else if (ReturnPropertyType.IsBuffer())
		{
			// We handle FVoxelGenericStructBuffer below
			Struct = FVoxelBuffer::MakeEmpty(ReturnPropertyType.GetInnerType())->GetStruct();
		}
		else if (ReturnPropertyType.IsStruct())
		{
			Struct = ReturnPropertyType.GetStruct();
		}
	}

	if (Struct)
	{
		CppStructOps = Struct->GetCppStructOps();
		StructureSize = Struct->GetStructureSize();
		bStructHasZeroConstructor = Struct->GetCppStructOps()->HasZeroConstructor();
	}
}

void UVoxelFunctionLibrary::Call(
	const FVoxelNode_UFunction& Node,
	const FCachedFunction& CachedFunction,
	const FVoxelGraphQuery InQuery,
	const TConstVoxelArrayView<FVoxelRuntimePinValue*> Values)
{
	FVoxelNodeStatScope StatScope(Node, 0);

	void* ReturnMemory = nullptr;
	if (CachedFunction.ReturnProperty)
	{
		if (CachedFunction.Struct)
		{
			if (CachedFunction.Struct == StaticStructFast<FVoxelRuntimePinValue>())
			{
				checkVoxelSlow(Values.Last());
				ReturnMemory = Values.Last();
			}
			else
			{
				ReturnMemory = FMemory::Malloc(CachedFunction.StructureSize);

				if (CachedFunction.bStructHasZeroConstructor)
				{
					FMemory::Memzero(ReturnMemory, CachedFunction.StructureSize);
				}
				else
				{
					CachedFunction.CppStructOps->Construct(ReturnMemory);
				}

				if (CachedFunction.Struct == StaticStructFast<FVoxelGenericStructBuffer>())
				{
					*static_cast<FVoxelGenericStructBuffer*>(ReturnMemory) = FVoxelGenericStructBuffer::MakeEmpty(*CachedFunction.ReturnPropertyType.GetInnerType().GetStruct());
				}
			}
		}
		else
		{
			constexpr int32 Size = FMath::Max(sizeof(FName), sizeof(uint64));
			ReturnMemory = FMemory_Alloca(Size);
			FMemory::Memzero(ReturnMemory, Size);
		}
	}

	int32 Num = 1;
	if (StatScope.IsEnabled() ||
		AreVoxelStatsEnabled())
	{
		for (const FVoxelRuntimePinValue* Value : Values)
		{
			if (!Value ||
				!Value->IsValid())
			{
				continue;
			}

			if (Value->CanBeCastedTo<FVoxelBuffer>())
			{
				Num = FMath::Max(Num, Value->Get<FVoxelBuffer>().Num_Slow());
			}
		}

		StatScope.SetCount(Num);
	}

	{
		VOXEL_SCOPE_COUNTER_FORMAT("%s Num=%d", *CachedFunction.Function.GetName(), Num);
		VOXEL_SCOPE_COUNTER_FNAME(CachedFunction.Function.GetFName());

		TVoxelTypeCompatibleBytes<UVoxelFunctionLibrary> FunctionLibraryBytes;
#if VOXEL_DEBUG
		FMemory::Memzero(FunctionLibraryBytes);
#endif

		UVoxelFunctionLibrary& FunctionLibrary = FunctionLibraryBytes.GetValue();
		FunctionLibrary.Query = InQuery;
		FunctionLibrary.PrivateNode = &Node;

		FFrame Frame;
		Frame.Values = Values;
		CachedFunction.NativeFunc(&FunctionLibrary, Frame, ReturnMemory);
	}

	if (!CachedFunction.ReturnProperty)
	{
		return;
	}

	if (CachedFunction.Struct == StaticStructFast<FVoxelRuntimePinValue>())
	{
		checkVoxelSlow(ReturnMemory == Values.Last());
		return;
	}

	checkVoxelSlow(Values.Last());

	FVoxelRuntimePinValue& ReturnValue = *Values.Last();
	ReturnValue.Type = CachedFunction.ReturnPropertyType;

	if (CachedFunction.Struct)
	{
		ReturnValue.SharedStructType = CachedFunction.Struct;
		ReturnValue.SharedStruct = MakeShareableStruct(CachedFunction.Struct, ReturnMemory);
	}
	else
	{
		switch (ReturnValue.Type.GetInternalType())
		{
		default: VOXEL_ASSUME(false);
		case EVoxelPinInternalType::Bool: ReturnValue.bBool = *static_cast<const bool*>(ReturnMemory); break;
		case EVoxelPinInternalType::Float: ReturnValue.Float = *static_cast<const float*>(ReturnMemory); break;
		case EVoxelPinInternalType::Double: ReturnValue.Double = *static_cast<const double*>(ReturnMemory); break;
		case EVoxelPinInternalType::UInt16: ReturnValue.UInt16 = *static_cast<const uint16*>(ReturnMemory); break;
		case EVoxelPinInternalType::Int32: ReturnValue.Int32 = *static_cast<const int32*>(ReturnMemory); break;
		case EVoxelPinInternalType::Int64: ReturnValue.Int64 = *static_cast<const int64*>(ReturnMemory); break;
		case EVoxelPinInternalType::Name: ReturnValue.Name = *static_cast<const FName*>(ReturnMemory); break;
		case EVoxelPinInternalType::Byte: ReturnValue.Byte = *static_cast<const uint8*>(ReturnMemory); break;
		case EVoxelPinInternalType::Class: ReturnValue.Class = *static_cast<UClass* const*>(ReturnMemory); break;
		}
	}

	if (StatScope.IsEnabled() &&
		ReturnValue.CanBeCastedTo<FVoxelBuffer>())
	{
		StatScope.SetCount(FMath::Max(Num, ReturnValue.Get<FVoxelBuffer>().Num_Slow()));
	}
}