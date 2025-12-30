// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBuffer.h"
#include "VoxelGraphQuery.h"
#include "VoxelBufferAccessor.h"
#include "VoxelRuntimePinValue.h"
#include "VoxelFunctionLibrary.generated.h"

struct FVoxelNode_UFunction;

#undef PARAM_PASSED_BY_REF
#define PARAM_PASSED_BY_REF(ParamName, PropertyType, ParamType) \
	ParamType& ParamName = INLINE_LAMBDA -> ParamType& \
	{ \
		if constexpr (std::derived_from<ThisClass, UVoxelFunctionLibrary>) \
		{ \
			return Stack.StepCompiledInRef<PropertyType, ParamType>(nullptr); \
		} \
		else \
		{ \
			ParamType* ParamName##Temp = static_cast<ParamType*>(FMemory_Alloca(sizeof(ParamType))); \
			new (ParamName##Temp) ParamType(); \
			return Stack.StepCompiledInRef<PropertyType, ParamType>(ParamName##Temp); \
		} \
	};

#undef PARAM_PASSED_BY_REF_ZEROED
#define PARAM_PASSED_BY_REF_ZEROED(ParamName, PropertyType, ParamType) \
	ParamType& ParamName = INLINE_LAMBDA -> ParamType& \
	{ \
		if constexpr (std::derived_from<ThisClass, UVoxelFunctionLibrary>) \
		{ \
			return Stack.StepCompiledInRef<PropertyType, ParamType>(nullptr); \
		} \
		else \
		{ \
			ParamType* ParamName##Temp = static_cast<ParamType*>(FMemory_Alloca(sizeof(ParamType))); \
			new (ParamName##Temp) ParamType((ParamType)0); \
			return Stack.StepCompiledInRef<PropertyType, ParamType>(ParamName##Temp); \
		} \
	};

UCLASS()
class VOXELGRAPH_API UVoxelFunctionLibrary
	: public UObject
	, public FVoxelBufferInitializers
{
	GENERATED_BODY()

public:
	struct FFrameOverride
	{
		TConstVoxelArrayView<FVoxelRuntimePinValue*> Values;
		int32 Index = 0;

		template<typename PropertyType, typename NativeType>
		FORCEINLINE void StepCompiledIn(NativeType* RESTRICT OutData)
		{
			FVoxelRuntimePinValue* Value = Values[Index++];
			checkVoxelSlow(Value);

			if constexpr (
				std::is_same_v<PropertyType, FBoolProperty> &&
				std::is_same_v<NativeType, uint32>)
			{
				*OutData = Value->Get<bool>();
			}
			else
			{
				static_assert(!FVoxelRuntimePinValue::IsStructValue<NativeType>, "All structs should be passed by ref in voxel functions");
				*OutData = Value->Get<NativeType>();
			}
		}
		template<typename, typename NativeType>
		FORCEINLINE NativeType& StepCompiledInRef(void* TemporaryBuffer)
		{
			static_assert(!std::is_same_v<NativeType, FName>, "Cannot pass FName by ref in voxel functions: either pass it by value or use FVoxelNameWrapper");

			checkVoxelSlow(!TemporaryBuffer);

			FVoxelRuntimePinValue* Value = Values[Index++];
			checkVoxelSlow(Value);

			if constexpr (std::is_same_v<NativeType, FVoxelRuntimePinValue>)
			{
				return *Value;
			}
			else
			{
				return ConstCast(Value->Get<NativeType>());
			}
		}
	};
	VOXEL_UFUNCTION_OVERRIDE(FFrameOverride);

public:
	FVoxelGraphQuery Query;

	void RaiseBufferError() const;
	TSharedRef<FVoxelMessageToken> CreateMessageToken() const;

private:
	const FVoxelNode_UFunction* PrivateNode = nullptr;

public:
	static FVoxelPinType MakeType(const FProperty& Property);

	struct FCachedFunction
	{
		const UFunction& Function;
		FNativeFuncPtr NativeFunc;
		const FProperty* ReturnProperty;
		FVoxelPinType ReturnPropertyType;
		UScriptStruct* Struct = nullptr;
		UScriptStruct::ICppStructOps* CppStructOps = nullptr;
		int32 StructureSize = 0;
		bool bStructHasZeroConstructor = false;

		explicit FCachedFunction(const UFunction& Function);
	};
	static void Call(
		const FVoxelNode_UFunction& Node,
		const FCachedFunction& CachedFunction,
		FVoxelGraphQuery InQuery,
		TConstVoxelArrayView<FVoxelRuntimePinValue*> Values);
};