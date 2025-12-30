// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "MegaMaterial/MaterialExpressionVoxelMegaMaterialInternalSwitch.h"
#include "MegaMaterial/VoxelMegaMaterialSwitch.h"
#include "MaterialCompiler.h"
#include "Engine/Texture2D.h"
#include "VoxelHLSLMaterialTranslator.h"
#include "Materials/MaterialAttributeDefinitionMap.h"

UMaterialExpressionVoxelMegaMaterialInternalSwitch::UMaterialExpressionVoxelMegaMaterialInternalSwitch()
{
	Outputs.Reset();
	Outputs.Add(FExpressionOutput("Attributes"));
	Outputs.Add(FExpressionOutput("BaseColor"));
	Outputs.Add(FExpressionOutput("Specular"));
	Outputs.Add(FExpressionOutput("Roughness"));
	Outputs.Add(FExpressionOutput("Normal"));
	Outputs.Add(FExpressionOutput("WorldHeight"));
	Outputs.Add(FExpressionOutput("Opacity"));
	Outputs.Add(FExpressionOutput("Mask"));
	Outputs.Add(FExpressionOutput("Displacement"));
	Outputs.Add(FExpressionOutput("Mask4"));
}

UObject* UMaterialExpressionVoxelMegaMaterialInternalSwitch::GetReferencedTexture() const
{
	return FVoxelTextureUtilities::GetDefaultTexture2D();
}

#if WITH_EDITOR
uint32 UMaterialExpressionVoxelMegaMaterialInternalSwitch::GetOutputType(const int32 OutputIndex)
{
	if (OutputIndex == 0)
	{
		return MCT_MaterialAttributes;
	}
	if (OutputIndex == 1)
	{
		return MCT_Float3;
	}
	if (OutputIndex == 2)
	{
		return MCT_Float;
	}
	if (OutputIndex == 3)
	{
		return MCT_Float;
	}
	if (OutputIndex == 4)
	{
		return MCT_Float3;
	}
	if (OutputIndex == 5)
	{
		return MCT_Float;
	}
	if (OutputIndex == 6)
	{
		return MCT_Float;
	}
	if (OutputIndex == 7)
	{
		return MCT_Float;
	}
	if (OutputIndex == 8)
	{
		return MCT_Float;
	}
	if (OutputIndex == 9)
	{
		return MCT_Float4;
	}

	ensureVoxelSlow(false);
	return {};
}

bool UMaterialExpressionVoxelMegaMaterialInternalSwitch::IsResultMaterialAttributes(const int32 OutputIndex)
{
	return OutputIndex == 0;
}

int32 UMaterialExpressionVoxelMegaMaterialInternalSwitch::Compile(FMaterialCompiler* Compiler, const int32 OutputIndex)
{
	switch (OutputIndex)
	{
	default: ensure(false);
		return -1;
	case 0: return FVoxelMegaMaterialSwitch(*this, *Compiler).Compile();
	case 1: return FVoxelMegaMaterialSwitch(*this, *Compiler).CompileRVT(&FVoxelMegaMaterialSwitchInputs::BaseColor, MCT_Float3);
	case 2: return FVoxelMegaMaterialSwitch(*this, *Compiler).CompileRVT(&FVoxelMegaMaterialSwitchInputs::Specular, MCT_Float);
	case 3: return FVoxelMegaMaterialSwitch(*this, *Compiler).CompileRVT(&FVoxelMegaMaterialSwitchInputs::Roughness, MCT_Float);
	case 4: return FVoxelMegaMaterialSwitch(*this, *Compiler).CompileRVT(&FVoxelMegaMaterialSwitchInputs::Normal, MCT_Float3);
	case 5: return FVoxelMegaMaterialSwitch(*this, *Compiler).CompileRVT(&FVoxelMegaMaterialSwitchInputs::WorldHeight, MCT_Float);
	case 6: return FVoxelMegaMaterialSwitch(*this, *Compiler).CompileRVT(&FVoxelMegaMaterialSwitchInputs::Opacity, MCT_Float);
	case 7: return FVoxelMegaMaterialSwitch(*this, *Compiler).CompileRVT(&FVoxelMegaMaterialSwitchInputs::Mask, MCT_Float);
	case 8: return FVoxelMegaMaterialSwitch(*this, *Compiler).CompileRVT(&FVoxelMegaMaterialSwitchInputs::Displacement, MCT_Float);
	case 9: return FVoxelMegaMaterialSwitch(*this, *Compiler).CompileRVT(&FVoxelMegaMaterialSwitchInputs::Mask4, MCT_Float4);
	}
}

void UMaterialExpressionVoxelMegaMaterialInternalSwitch::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add("Voxel Mega Material Switch");
}

FName UMaterialExpressionVoxelMegaMaterialInternalSwitch::GetInputName(const int32 InputIndex) const
{
	const FString Name = INLINE_LAMBDA
	{
	switch (InputIndex % 10)
	{
	default: ensure(false);
	case 0: return "Attributes";
	case 1: return "BaseColor";
	case 2: return "Specular";
	case 3: return "Roughness";
	case 4: return "Normal";
	case 5: return "WorldHeight";
	case 6: return "Opacity";
	case 7: return "Mask";
	case 8: return "Displacement";
	case 9: return "Mask4";
	}
	};

	return FName(FString::Printf(TEXT("Output %d %s"), InputIndex, *Name));
}

FExpressionInput* UMaterialExpressionVoxelMegaMaterialInternalSwitch::GetInput(const int32 InputIndex)
{
	if (!IndexToInputs.Contains(FVoxelMaterialRenderIndex(InputIndex / 10)))
	{
		return nullptr;
	}
	FVoxelMegaMaterialSwitchInputs& Inputs = IndexToInputs[FVoxelMaterialRenderIndex(InputIndex / 10)];

	switch (InputIndex % 10)
	{
	default: ensure(false); return nullptr;
	case 0: return &Inputs.Attributes;
	case 1: return &Inputs.BaseColor;
	case 2: return &Inputs.Specular;
	case 3: return &Inputs.Roughness;
	case 4: return &Inputs.Normal;
	case 5: return &Inputs.WorldHeight;
	case 6: return &Inputs.Opacity;
	case 7: return &Inputs.Mask;
	case 8: return &Inputs.Displacement;
	case 9: return &Inputs.Mask4;
	}
}

TArrayView<FExpressionInput*> UMaterialExpressionVoxelMegaMaterialInternalSwitch::GetInputsView()
{
	CachedInputs.Empty();
	CachedInputs.Reserve(IndexToInputs.Num());

	for (auto& It : IndexToInputs)
	{
		CachedInputs.Add(&It.Value.Attributes);
		CachedInputs.Add(&It.Value.BaseColor);
		CachedInputs.Add(&It.Value.Specular);
		CachedInputs.Add(&It.Value.Roughness);
		CachedInputs.Add(&It.Value.Normal);
		CachedInputs.Add(&It.Value.WorldHeight);
		CachedInputs.Add(&It.Value.Opacity);
		CachedInputs.Add(&It.Value.Mask);
		CachedInputs.Add(&It.Value.Displacement);
		CachedInputs.Add(&It.Value.Mask4);
	}

	return CachedInputs;
}
#endif