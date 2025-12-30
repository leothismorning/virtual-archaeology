// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelComputeNode.h"
#include "Buffer/VoxelFloatBuffers.h"
#include "VoxelNoiseNodes.generated.h"

USTRUCT(Category = "Math|Seed", meta = (CompactNodeTitle = "MIX"))
struct VOXELGRAPH_API FVoxelComputeNode_MixSeeds : public FVoxelComputeNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_TEMPLATE_INPUT_PIN(FVoxelSeed, A, nullptr);
	VOXEL_TEMPLATE_INPUT_PIN(FVoxelSeed, B, nullptr);
	VOXEL_TEMPLATE_OUTPUT_PIN(FVoxelSeed, ReturnValue);

	virtual FString GenerateCode(FCode& Code) const override
	{
		return "{ReturnValue} = MurmurHash32(MurmurHash32({A}) ^ {B})";
	}
};

USTRUCT(Category = "Math|Seed", meta = (ShowInShortList))
struct VOXELGRAPH_API FVoxelNode_MakeSeeds : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_TEMPLATE_INPUT_PIN(FVoxelSeed, Seed, nullptr);

	FVoxelNode_MakeSeeds()
	{
		FixupSeedPins();
	}

	//~ Begin FVoxelNode Interface
	virtual void Initialize(FInitializer& Initializer) override;
	virtual void Compute(FVoxelGraphQuery Query) const override;
	virtual void PostSerialize() override;
	//~ End FVoxelNode Interface

public:
	TVoxelArray<FPinRef_Output> ResultPins;

	UPROPERTY()
	int32 NumNewSeeds = 1;

	void FixupSeedPins();

#if WITH_EDITOR
	class FDefinition : public Super::FDefinition
	{
	public:
		GENERATED_VOXEL_NODE_DEFINITION_BODY(FVoxelNode_MakeSeeds);

		virtual bool CanAddInputPin() const override
		{
			return true;
		}
		virtual void AddInputPin() override
		{
			Node.NumNewSeeds++;
			Node.FixupSeedPins();
		}

		virtual bool CanRemoveInputPin() const override
		{
			return Node.NumNewSeeds > 1;
		}
		virtual void RemoveInputPin() override
		{
			Node.NumNewSeeds--;
			Node.FixupSeedPins();
		}
	};
#endif
};

USTRUCT(Category = "Noise")
struct VOXELGRAPH_API FVoxelComputeNode_PerlinNoise2D : public FVoxelComputeNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelVector2DBuffer, Position, nullptr, PositionPin);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Amplitude, 10000.f);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, FeatureScale, 100000.f);
	VOXEL_INPUT_PIN(FVoxelSeedBuffer, Seed, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Value);

	virtual FString GenerateCode(FCode& Code) const override
	{
		Code.AddInclude("VoxelNoiseNodesImpl.isph");
		return "{Value} = GetPerlin2D({Seed}, {Position} / {FeatureScale}) * {Amplitude}";
	}
};

USTRUCT(Category = "Noise")
struct VOXELGRAPH_API FVoxelComputeNode_PerlinNoise3D : public FVoxelComputeNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelVectorBuffer, Position, nullptr, PositionPin);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Amplitude, 10000.f);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, FeatureScale, 100000.f);
	VOXEL_INPUT_PIN(FVoxelSeedBuffer, Seed, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Value);

	virtual FString GenerateCode(FCode& Code) const override
	{
		Code.AddInclude("VoxelNoiseNodesImpl.isph");
		return "{Value} = GetPerlin3D({Seed}, {Position} / {FeatureScale}) * {Amplitude}";
	}
};

// Two dimensional cellular noise
USTRUCT(Category = "Noise")
struct VOXELGRAPH_API FVoxelComputeNode_CellularNoise2D : public FVoxelComputeNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	// Position at which to calculate output noise
	VOXEL_INPUT_PIN(FVoxelVector2DBuffer, Position, nullptr, PositionPin);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Amplitude, 10000.f);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, FeatureScale, 100000.f);
	// How irregular the cells are, needs to be between 0 and 1 to avoid glitches
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Jitter, 0.9f);
	// Used to randomize the output noise
	VOXEL_INPUT_PIN(FVoxelSeedBuffer, Seed, nullptr);
	// Distance to nearest cell center at position
	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Value);

	virtual FString GenerateCode(FCode& Code) const override
	{
		Code.AddInclude("VoxelNoiseNodesImpl.isph");
		return "{Value} = GetCellularNoise2D({Seed}, {Position} / {FeatureScale}, {Jitter}) * {Amplitude}";
	}
};

// Three dimensional cellular noise
USTRUCT(Category = "Noise")
struct VOXELGRAPH_API FVoxelComputeNode_CellularNoise3D : public FVoxelComputeNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	// Position at which to calculate output noise
	VOXEL_INPUT_PIN(FVoxelVectorBuffer, Position, nullptr, PositionPin);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Amplitude, 10000.f);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, FeatureScale, 100000.f);
	// How irregular the cells are, needs to be between 0 and 1 to avoid glitches
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Jitter, 0.9f);
	// Used to randomize the output noise
	VOXEL_INPUT_PIN(FVoxelSeedBuffer, Seed, nullptr);
	// Distance to nearest cell center at position
	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Value);

	virtual FString GenerateCode(FCode& Code) const override
	{
		Code.AddInclude("VoxelNoiseNodesImpl.isph");
		return "{Value} = GetCellularNoise3D({Seed}, {Position} / {FeatureScale}, {Jitter}) * {Amplitude}";
	}
};

USTRUCT(Category = "Noise")
struct VOXELGRAPH_API FVoxelComputeNode_TrueDistanceCellularNoise2D : public FVoxelComputeNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelVector2DBuffer, Position, nullptr, PositionPin);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Amplitude, 10000.f);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, FeatureScale, 100000.f);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Jitter, 0.9f);
	VOXEL_INPUT_PIN(FVoxelSeedBuffer, Seed, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Value);
	VOXEL_OUTPUT_PIN(FVoxelVector2DBuffer, CellPosition);

	virtual FString GenerateCode(FCode& Code) const override
	{
		Code.AddInclude("VoxelNoiseNodesImpl.isph");
		return
			"{Value} = GetTrueDistanceCellularNoise2D({Seed}, {Position} / {FeatureScale}, {Jitter}, &{CellPosition}) * {Amplitude};"
			"{CellPosition} = {CellPosition} * {FeatureScale}";
	}
};

USTRUCT(Category = "Noise")
struct VOXELGRAPH_API FVoxelComputeNode_TrueDistanceCellularNoise3D : public FVoxelComputeNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelVectorBuffer, Position, nullptr, PositionPin);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Amplitude, 10000.f);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, FeatureScale, 100000.f);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Jitter, 0.9f);
	VOXEL_INPUT_PIN(FVoxelSeedBuffer, Seed, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Value);
	VOXEL_OUTPUT_PIN(FVoxelVectorBuffer, CellPosition);

	virtual FString GenerateCode(FCode& Code) const override
	{
		Code.AddInclude("VoxelNoiseNodesImpl.isph");
		return
			"{Value} = GetTrueDistanceCellularNoise3D({Seed}, {Position} / {FeatureScale}, {Jitter}, &{CellPosition}) * {Amplitude};"
			"{CellPosition} = {CellPosition} * {FeatureScale}";
	}
};

USTRUCT(Category = "Noise")
struct VOXELGRAPH_API FVoxelComputeNode_SimplexNoise2D : public FVoxelComputeNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelVector2DBuffer, Position, nullptr, PositionPin);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Amplitude, 10000.f);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, FeatureScale, 100000.f);
	VOXEL_INPUT_PIN(FVoxelSeedBuffer, Seed, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Value);

	virtual FString GenerateCode(FCode& Code) const override
	{
		Code.AddInclude("VoxelNoiseNodesImpl.isph");
		return "{Value} = GetSimplex2D({Seed}, {Position} / {FeatureScale}) * {Amplitude}";
	}
};

USTRUCT(Category = "Noise")
struct VOXELGRAPH_API FVoxelComputeNode_SimplexNoise3D : public FVoxelComputeNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelVectorBuffer, Position, nullptr, PositionPin);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Amplitude, 10000.f);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, FeatureScale, 100000.f);
	VOXEL_INPUT_PIN(FVoxelSeedBuffer, Seed, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Value);

	virtual FString GenerateCode(FCode& Code) const override
	{
		Code.AddInclude("VoxelNoiseNodesImpl.isph");
		return "{Value} = GetSimplex3D({Seed}, {Position} / {FeatureScale}) * {Amplitude}";
	}
};

USTRUCT(Category = "Noise")
struct VOXELGRAPH_API FVoxelComputeNode_ValueNoise2D : public FVoxelComputeNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelVector2DBuffer, Position, nullptr, PositionPin);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Amplitude, 10000.f);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, FeatureScale, 100000.f);
	VOXEL_INPUT_PIN(FVoxelSeedBuffer, Seed, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Value);

	virtual FString GenerateCode(FCode& Code) const override
	{
		Code.AddInclude("VoxelNoiseNodesImpl.isph");
		return "{Value} = GetValue2D({Seed}, {Position} / {FeatureScale}) * {Amplitude}";
	}
};

USTRUCT(Category = "Noise")
struct VOXELGRAPH_API FVoxelComputeNode_ValueNoise3D : public FVoxelComputeNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelVectorBuffer, Position, nullptr, PositionPin);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Amplitude, 10000.f);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, FeatureScale, 100000.f);
	VOXEL_INPUT_PIN(FVoxelSeedBuffer, Seed, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Value);

	virtual FString GenerateCode(FCode& Code) const override
	{
		Code.AddInclude("VoxelNoiseNodesImpl.isph");
		return "{Value} = GetValue3D({Seed}, {Position} / {FeatureScale}) * {Amplitude}";
	}
};