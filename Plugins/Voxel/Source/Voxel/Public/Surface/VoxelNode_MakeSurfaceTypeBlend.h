// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelSurfaceTypeBuffer.h"
#include "VoxelSurfaceTypeBlendBuffer.h"
#include "Buffer/VoxelBaseBuffers.h"
#include "VoxelNode_MakeSurfaceTypeBlend.generated.h"

USTRUCT(Category = "Surface Type")
struct VOXEL_API FVoxelNode_MakeSurfaceTypeBlend : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	VOXEL_OUTPUT_PIN(FVoxelSurfaceTypeBlendBuffer, SurfaceTypeBlend);

public:
	FVoxelNode_MakeSurfaceTypeBlend();

	//~ Begin FVoxelNode Interface
	virtual void Initialize(FInitializer& Initializer) override;
	virtual void Compute(FVoxelGraphQuery Query) const override;
	virtual void PostSerialize() override;
	//~ End FVoxelNode Interface

public:
	struct FLayerPin
	{
		TPinRef_Input<FVoxelSurfaceTypeBuffer> Type;
		TPinRef_Input<FVoxelFloatBuffer> Weight;
	};
	TVoxelArray<FLayerPin> LayerPins;

	UPROPERTY()
	int32 NumLayerPins = 2;

	void FixupLayerPins();

public:
#if WITH_EDITOR
	class FDefinition : public Super::FDefinition
	{
	public:
		GENERATED_VOXEL_NODE_DEFINITION_BODY(FVoxelNode_MakeSurfaceTypeBlend);

		virtual FString GetAddPinLabel() const override;
		virtual FString GetAddPinTooltip() const override;

		virtual bool CanAddInputPin() const override;
		virtual void AddInputPin() override;

		virtual bool CanRemoveInputPin() const override;
		virtual void RemoveInputPin() override;
	};
#endif
};