// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelComputeNode.h"
#include "VoxelTemplateNode.h"
#include "VoxelTemplatedMathNodes.generated.h"

USTRUCT(meta = (Abstract))
struct VOXELGRAPH_API FVoxelTemplateNode_FloatMathNode : public FVoxelTemplateNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_TEMPLATE_OUTPUT_PIN(FVoxelWildcard, ReturnValue);

public:
	virtual FPin* ExpandPins(FNode& Node, TArray<FPin*> Pins, const TArray<FPin*>& AllPins) const override;

#if WITH_EDITOR
	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;
#endif

	virtual UScriptStruct* GetFloatInnerNode() const VOXEL_PURE_VIRTUAL({});
	virtual UScriptStruct* GetDoubleInnerNode() const VOXEL_PURE_VIRTUAL({});
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct VOXELGRAPH_API FVoxelComputeNode_Frac_Float : public FVoxelComputeNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_TEMPLATE_INPUT_PIN(float, Value, nullptr);
	VOXEL_TEMPLATE_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(FCode& Code) const override
	{
		return "{ReturnValue} = {Value} - floor({Value})";
	}
};

USTRUCT(meta = (Internal))
struct VOXELGRAPH_API FVoxelComputeNode_Frac_Double : public FVoxelComputeNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_TEMPLATE_INPUT_PIN(double, Value, nullptr);
	VOXEL_TEMPLATE_OUTPUT_PIN(double, ReturnValue);

	virtual FString GenerateCode(FCode& Code) const override
	{
		return "{ReturnValue} = {Value} - floor({Value})";
	}
};

// Returns the fractional part of a float.
USTRUCT(Category = "Math|Float")
struct VOXELGRAPH_API FVoxelTemplateNode_Frac : public FVoxelTemplateNode_FloatMathNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_TEMPLATE_INPUT_PIN(FVoxelWildcard, A, nullptr);

	virtual UScriptStruct* GetFloatInnerNode() const override
	{
		return FVoxelComputeNode_Frac_Float::StaticStruct();
	}
	virtual UScriptStruct* GetDoubleInnerNode() const override
	{
		return FVoxelComputeNode_Frac_Double::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct VOXELGRAPH_API FVoxelComputeNode_Power_Float : public FVoxelComputeNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	// If negative, abs(Base) will be used instead
	VOXEL_TEMPLATE_INPUT_PIN(float, Base, nullptr);
	// Exponent
	VOXEL_TEMPLATE_INPUT_PIN(float, Exp, nullptr);
	// Result
	VOXEL_TEMPLATE_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(FCode& Code) const override
	{
		// Base cannot be negative
		return "{ReturnValue} = pow(abs({Base}), {Exp})";
	}
};

USTRUCT(meta = (Internal))
struct VOXELGRAPH_API FVoxelComputeNode_Power_Double : public FVoxelComputeNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	// If negative, abs(Base) will be used instead
	VOXEL_TEMPLATE_INPUT_PIN(double, Base, nullptr);
	// Exponent
	VOXEL_TEMPLATE_INPUT_PIN(double, Exp, nullptr);
	// Result
	VOXEL_TEMPLATE_OUTPUT_PIN(double, ReturnValue);

	virtual FString GenerateCode(FCode& Code) const override
	{
		// Base cannot be negative
		return "{ReturnValue} = pow(abs({Base}), {Exp})";
	}
};

// Power (Base to the Exp-th power)
USTRUCT(Category = "Math|Float")
struct VOXELGRAPH_API FVoxelTemplateNode_Power : public FVoxelTemplateNode_FloatMathNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_TEMPLATE_INPUT_PIN(FVoxelWildcard, Base, nullptr);
	VOXEL_TEMPLATE_INPUT_PIN(FVoxelWildcard, Exp, nullptr);

	virtual UScriptStruct* GetFloatInnerNode() const override
	{
		return FVoxelComputeNode_Power_Float::StaticStruct();
	}
	virtual UScriptStruct* GetDoubleInnerNode() const override
	{
		return FVoxelComputeNode_Power_Double::StaticStruct();
	}
};