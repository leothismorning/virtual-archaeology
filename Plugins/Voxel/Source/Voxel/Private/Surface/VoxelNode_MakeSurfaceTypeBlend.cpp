// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Surface/VoxelNode_MakeSurfaceTypeBlend.h"
#include "VoxelBufferAccessor.h"
#include "VoxelGraphMigration.h"

VOXEL_RUN_ON_STARTUP_GAME()
{
	REGISTER_VOXEL_NODE_MIGRATION("PackMaterial", FVoxelNode_MakeSurfaceTypeBlend);
}

FVoxelNode_MakeSurfaceTypeBlend::FVoxelNode_MakeSurfaceTypeBlend()
{
	FixupLayerPins();
}

void FVoxelNode_MakeSurfaceTypeBlend::Initialize(FInitializer& Initializer)
{
	VOXEL_FUNCTION_COUNTER();

	for (FLayerPin& LayerPin : LayerPins)
	{
		Initializer.InitializePinRef(LayerPin.Type);
		Initializer.InitializePinRef(LayerPin.Weight);
	}
}

void FVoxelNode_MakeSurfaceTypeBlend::Compute(const FVoxelGraphQuery Query) const
{
	struct FLayer
	{
		TValue<FVoxelSurfaceTypeBuffer> Types;
		TValue<FVoxelFloatBuffer> Weights;
	};

	TVoxelArray<FLayer> Layers;
	Layers.Reserve(LayerPins.Num());

	for (const FLayerPin& LayerPin : LayerPins)
	{
		Layers.Add_EnsureNoGrow(FLayer
		{
			LayerPin.Type.Get(Query),
			LayerPin.Weight.Get(Query)
		});
	}

	Query.AddTask([this, Query, Layers = MoveTemp(Layers)]
	{
		VOXEL_FUNCTION_COUNTER();

		int32 Num = 1;
		for (const FLayer& Layer : Layers)
		{
			if (!FVoxelBufferAccessor::MergeNum(Num, *Layer.Types) ||
				!FVoxelBufferAccessor::MergeNum(Num, *Layer.Weights))
			{
				RaiseBufferError();
				return;
			}
		}

		const TSharedRef<FVoxelSurfaceTypeBlendBuffer> Result = MakeShared<FVoxelSurfaceTypeBlendBuffer>();
		Result->Allocate(Num);

		TVoxelInlineArray<FVoxelSurfaceTypeBlendLayer, FVoxelSurfaceTypeBlend::MaxLayers> LocalLayers;

		for (int32 Index = 0; Index < Num; Index++)
		{
			FVoxelSurfaceTypeBlend& SurfaceTypeBlend = Result->View()[Index];

			LocalLayers.Reset();

			for (const FLayer& Layer : Layers)
			{
				const float Weight = (*Layer.Weights)[Index];
				if (Weight <= 0.f)
				{
					continue;
				}

				const FVoxelSurfaceType Type = (*Layer.Types)[Index];

				INLINE_LAMBDA
				{
					for (FVoxelSurfaceTypeBlendLayer& LocalLayer : LocalLayers)
					{
						if (LocalLayer.Type == Type)
						{
							LocalLayer.Weight += Weight;
							return;
						}
					}

					LocalLayers.Add(FVoxelSurfaceTypeBlendLayer(Type, Weight));
				};
			}

			SurfaceTypeBlend.InitializeFromLayers(LocalLayers);
		}

		SurfaceTypeBlendPin.Set(Query, Result);
	});
}

void FVoxelNode_MakeSurfaceTypeBlend::PostSerialize()
{
	Super::PostSerialize();

	FixupLayerPins();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNode_MakeSurfaceTypeBlend::FixupLayerPins()
{
	VOXEL_FUNCTION_COUNTER();

	for (const FLayerPin& Layer : LayerPins)
	{
		RemovePin(Layer.Type.GetName());
		RemovePin(Layer.Weight.GetName());
	}
	LayerPins.Reset();

	for (int32 Index = 0; Index < NumLayerPins; Index++)
	{
		const FString Category = "Layer " + FString::FromInt(Index);

		LayerPins.Add(FLayerPin
		{
			CreateInputPin<FVoxelSurfaceTypeBuffer>(
				FName("Type", Index + 1),
				VOXEL_PIN_METADATA(
					FVoxelSurfaceTypeBlendBuffer,
					nullptr,
					DisplayName("Type"),
					Tooltip("Surface type of this layer"),
					Category(Category))),

			CreateInputPin<FVoxelFloatBuffer>(
				FName("Weight", Index + 1),
				VOXEL_PIN_METADATA(
					FVoxelFloatBuffer,
					1.f,
					DisplayName("Weight"),
					Tooltip("Weight of this layer"),
					Category(Category)))
		});
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
FString FVoxelNode_MakeSurfaceTypeBlend::FDefinition::GetAddPinLabel() const
{
	return "Add Layer";
}

FString FVoxelNode_MakeSurfaceTypeBlend::FDefinition::GetAddPinTooltip() const
{
	return "Adds an additional layer pin";
}

bool FVoxelNode_MakeSurfaceTypeBlend::FDefinition::CanAddInputPin() const
{
	return true;
}

void FVoxelNode_MakeSurfaceTypeBlend::FDefinition::AddInputPin()
{
	Node.NumLayerPins++;
	Node.FixupLayerPins();
}

bool FVoxelNode_MakeSurfaceTypeBlend::FDefinition::CanRemoveInputPin() const
{
	return Node.NumLayerPins > 1;
}

void FVoxelNode_MakeSurfaceTypeBlend::FDefinition::RemoveInputPin()
{
	Node.NumLayerPins--;
	Node.FixupLayerPins();
}
#endif