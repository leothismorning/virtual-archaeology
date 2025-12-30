// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelNodeLibrary.h"
#include "VoxelFunctionLibrary.h"
#include "Nodes/VoxelOutputNode.h"
#include "Nodes/VoxelNode_UFunction.h"

FVoxelNodeLibrary* GVoxelNodeLibrary = nullptr;

VOXEL_RUN_ON_STARTUP_EDITOR_COMMANDLET()
{
	GVoxelNodeLibrary = new FVoxelNodeLibrary();
	GOnVoxelModuleUnloaded_DoCleanup.AddLambda([]
	{
		delete GVoxelNodeLibrary;
		GVoxelNodeLibrary = nullptr;
	});

	FCoreUObjectDelegates::ReloadCompleteDelegate.AddLambda([](EReloadCompleteReason)
	{
		delete GVoxelNodeLibrary;
		GVoxelNodeLibrary = new FVoxelNodeLibrary();
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelNodeLibrary::FVoxelNodeLibrary()
{
	VOXEL_FUNCTION_COUNTER();

	for (UScriptStruct* Struct : GetDerivedStructs<FVoxelNode>())
	{
		const bool bSkip = INLINE_LAMBDA
		{
			if (Struct->HasMetaData(STATIC_FNAME("Abstract")))
			{
				return true;
			}

			if (!Struct->HasMetaDataHierarchical(STATIC_FNAME("Internal")))
			{
				return false;
			}

			if (FVoxelUtilities::IsDevWorkflow() &&
				Struct->IsChildOf(StaticStructFast<FVoxelOutputNode>()))
			{
				return false;
			}

			return true;
		};

		if (bSkip)
		{
			continue;
		}

		Nodes.Add(MakeSharedStruct<FVoxelNode>(Struct));
	}

	for (const TSharedRef<const FVoxelNode>& Node : Nodes)
	{
		StructToNode.Add_EnsureNew(Node->GetStruct(), Node);
	}

	for (const TSubclassOf<UVoxelFunctionLibrary>& Class : GetDerivedClasses<UVoxelFunctionLibrary>())
	{
		for (UFunction* Function : GetClassFunctions(Class))
		{
			if (Function->HasMetaData(STATIC_FNAME("Internal")))
			{
				continue;
			}

			const TSharedRef<FVoxelNode_UFunction> Node = MakeShared<FVoxelNode_UFunction>();
			Node->SetFunction_EditorOnly(Function);
			Nodes.Add(Node);
			FunctionToNode.Add_EnsureNew(Function, Node);
		}
	}
	Nodes.Shrink();

#if VOXEL_DEBUG
	for (const TSharedRef<const FVoxelNode>& Node : Nodes)
	{
		(void)Node->GetCategory();
		(void)Node->GetDisplayName();
		(void)Node->GetTooltip();
	}
#endif

	for (const TSharedRef<const FVoxelNode>& Node : Nodes)
	{
		if (!Node->GetMetadataContainer().HasMetaDataHierarchical(STATIC_FNAME("NativeMakeFunc")))
		{
			continue;
		}

		const FVoxelPin& OutputPin = Node->GetUniqueOutputPin();

		FVoxelPinTypeSet Types;
		if (OutputPin.IsPromotable())
		{
			Types = Node->GetPromotionTypes(OutputPin);
		}
		else
		{
			Types.Add(OutputPin.GetType());
		}

		for (const FVoxelPinType& Type : Types.GetExplicitTypes())
		{
			TypeToMakeNode.Add_EnsureNew(Type, Node);
		}
	}

	for (const TSharedRef<const FVoxelNode>& Node : Nodes)
	{
		if (!Node->GetMetadataContainer().HasMetaDataHierarchical(STATIC_FNAME("NativeBreakFunc")))
		{
			continue;
		}

		const FVoxelPin& InputPin = Node->GetUniqueInputPin();

		FVoxelPinTypeSet Types;
		if (InputPin.IsPromotable())
		{
			Types = Node->GetPromotionTypes(InputPin);
		}
		else
		{
			Types.Add(InputPin.GetType());
		}

		for (const FVoxelPinType& Type : Types.GetExplicitTypes())
		{
			ensure(!TypeToBreakNode.Contains(Type));
			TypeToBreakNode.Add_EnsureNew(Type, Node);
		}
	}

	for (const TSharedRef<const FVoxelNode>& Node : Nodes)
	{
		if (!Node->GetMetadataContainer().HasMetaDataHierarchical(STATIC_FNAME("Autocast")))
		{
			continue;
		}

		const TSharedRef<FVoxelNode> NodeCopy = Node->MakeSharedCopy();

		FVoxelPin* FirstInputPin = nullptr;
		FVoxelPin* FirstOutputPin = nullptr;
		for (FVoxelPin& Pin : NodeCopy->GetPins())
		{
			if (Pin.bIsInput && !FirstInputPin)
			{
				FirstInputPin = &Pin;
			}
			if (!Pin.bIsInput && !FirstOutputPin)
			{
				FirstOutputPin = &Pin;
			}
		}
		check(FirstInputPin);
		check(FirstOutputPin);

		FVoxelPinTypeSet InputTypes;
		if (FirstInputPin->IsPromotable())
		{
			InputTypes = NodeCopy->GetPromotionTypes(*FirstInputPin);
		}
		else
		{
			InputTypes.Add(FirstInputPin->GetType());
		}

		FVoxelPinTypeSet OutputTypes;
		if (FirstOutputPin->IsPromotable())
		{
			OutputTypes = NodeCopy->GetPromotionTypes(*FirstOutputPin);
		}
		else
		{
			OutputTypes.Add(FirstOutputPin->GetType());
		}

		TSet<TPair<FVoxelPinType, FVoxelPinType>> Pairs;
		for (const FVoxelPinType& InputType : InputTypes.GetExplicitTypes())
		{
			if (FirstInputPin->IsPromotable())
			{
				NodeCopy->PromotePin(*FirstInputPin, InputType);
			}

			for (const FVoxelPinType& OutputType : OutputTypes.GetExplicitTypes())
			{
				if (FirstOutputPin->IsPromotable())
				{
					NodeCopy->PromotePin(*FirstOutputPin, OutputType);
				}

				Pairs.Add({ FirstInputPin->GetType(), FirstOutputPin->GetType() });
			}
		}

		for (const TPair<FVoxelPinType, FVoxelPinType>& Pair : Pairs)
		{
			ensure(!FromTypeAndToTypeToCastNode.Contains(Pair));
			FromTypeAndToTypeToCastNode.Add_EnsureNew(Pair, Node);

			FVoxelPinType FromUniformType = Pair.Key.GetInnerType();
			if (FromUniformType == Pair.Key)
			{
				continue;
			}

			TPair<FVoxelPinType, FVoxelPinType> UniformCast = { FromUniformType, Pair.Value };
			ensure(!FromTypeAndToTypeToCastNode.Contains(UniformCast));
			FromTypeAndToTypeToCastNode.Add_EnsureNew(UniformCast, Node);
		}
	}
}

const FVoxelNodeLibrary& FVoxelNodeLibrary::Get()
{
	return *GVoxelNodeLibrary;
}