// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "MegaMaterial/VoxelMegaMaterialGenerator.h"
#include "MegaMaterial/VoxelMegaMaterial.h"
#include "MegaMaterial/MaterialExpressionGetVoxelRandom.h"
#include "MegaMaterial/MaterialExpressionGetVoxelMetadata.h"
#include "MegaMaterial/MaterialExpressionVoxelNaniteMaterialHook.h"
#include "MegaMaterial/MaterialExpressionVoxelMegaMaterialSwitch.h"
#include "MegaMaterial/MaterialExpressionVoxelMegaMaterialInternalSwitch.h"
#include "VoxelMaterialGenerator.h"
#include "Surface/VoxelSurfaceTypeAsset.h"

#if WITH_EDITOR
#include "MaterialDomain.h"
#include "MaterialShared.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialExpressionComment.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionShadingModel.h"
#include "Materials/MaterialAttributeDefinitionMap.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionSetMaterialAttributes.h"
#include "Materials/MaterialExpressionRuntimeVirtualTextureOutput.h"
#endif

#if WITH_EDITOR
DEFINE_PRIVATE_ACCESS(UMaterial, ShadingModel);

UClass* UMaterialExpressionRuntimeVirtualTextureOutput::GetPrivateStaticClass()
{
	static UClass* Class = FindObjectChecked<UClass>(nullptr, TEXT("/Script/Engine.MaterialExpressionRuntimeVirtualTextureOutput"));
	check(Class);
	return Class;
}
#endif

#if WITH_EDITOR
bool FVoxelMegaMaterialGenerator::GenerateMaterialForTarget(
	const UVoxelMegaMaterial& MegaMaterial,
	const TMap<FVoxelMaterialRenderIndex, FVoxelMegaMaterialSurfaceInfo>& IndexToSurfaceInfo,
	const EVoxelMegaMaterialTarget Target,
	UMaterial& NewMaterial,
	UMaterialInstanceConstant& NewMaterialInstance,
	const TConstVoxelArrayView<TObjectPtr<UVoxelMetadata>> Metadatas,
	TVoxelSet<TVoxelObjectPtr<UObject>>& OutWatchedMaterials,
	const bool bIsDummyMaterial)
{
	VOXEL_FUNCTION_COUNTER();
	check(NewMaterialInstance.Parent == &NewMaterial);

	UTexture2D* DitherNoiseTexture = MegaMaterial.DitherNoiseTexture.LoadSynchronous();

	TUniquePtr<FMaterialUpdateContext> UpdateContext;
	if (!bIsDummyMaterial)
	{
		UpdateContext = MakeUnique<FMaterialUpdateContext>();
		UpdateContext->AddMaterial(&NewMaterial);
		UpdateContext->AddMaterialInstance(&NewMaterialInstance);
	}

	NewMaterial.bUseMaterialAttributes = true;
	NewMaterial.bTangentSpaceNormal = false;
	NewMaterial.BlendMode = MegaMaterial.bGenerateMaskedMaterial ? BLEND_Masked : BLEND_Opaque;
	NewMaterial.bHasPixelAnimation = MegaMaterial.bSetHasPixelAnimation;
	NewMaterial.bEnableDisplacementFade = MegaMaterial.bEnableDisplacementFade;
	NewMaterial.DisplacementFadeRange = MegaMaterial.DisplacementFade;

	FVoxelUtilities::ClearMaterialExpressions(NewMaterial);

	TVoxelSet<EMaterialShadingModel> ShadingModels;
	for (const auto& It : IndexToSurfaceInfo)
	{
		UVoxelSurfaceTypeAsset& SurfaceType = *It.Value.SurfaceType;
		UMaterial& Material = *SurfaceType.Material->GetMaterial();
		const EMaterialShadingModel ShadingModel = PrivateAccess::ShadingModel(Material);

		ShadingModels.Add(ShadingModel);
	}

	if (ShadingModels.Num() == 1)
	{
		PrivateAccess::ShadingModel(NewMaterial) = ShadingModels.GetUniqueValue();
	}
	else
	{
		PrivateAccess::ShadingModel(NewMaterial) = MSM_FromMaterialExpression;

		if (ShadingModels.Contains(MSM_Unlit))
		{
			VOXEL_MESSAGE(Error, "{0}: unlit materials cannot be used with lit materials", MegaMaterial);
		}
	}

	TVoxelArray<TFunction<void(FMaterialInstanceParameterUpdateContext&)>> DelayedCopyParameterValues;

	if (MegaMaterial.CustomOutputsMaterial &&
		MegaMaterial.CustomOutputsMaterial->GetMaterial())
	{
		CollectedWatchedMaterials(
			*MegaMaterial.CustomOutputsMaterial,
			OutWatchedMaterials);

		VOXEL_SCOPE_COUNTER_FNAME(MegaMaterial.CustomOutputsMaterial->GetFName());

		const FString Prefix = "VOXELCUSTOMOUTPUT_";

		FVoxelMaterialGenerator Generator(
			MegaMaterial,
			NewMaterial,
			"VOXELCUSTOMOUTPUT_",
			false,
			ShouldDuplicateFunction);

		if (!ensure(Generator.CopyExpressions(*MegaMaterial.CustomOutputsMaterial->GetMaterial())))
		{
			return false;
		}

		DelayedCopyParameterValues.Add([&MegaMaterial, &NewMaterialInstance, Prefix](FMaterialInstanceParameterUpdateContext& InstanceUpdateContext)
		{
			FVoxelUtilities::CopyParameterValues(
				InstanceUpdateContext,
				NewMaterialInstance,
				*MegaMaterial.CustomOutputsMaterial,
				Prefix);
		});

		if (const FVoxelOptionalIntBox2D Bounds = Generator.GetBounds())
		{
			Generator.MoveExpressions(FIntPoint
			{
				-700 - Bounds->Max.X,
				-1000 - Bounds->Size().Y - Bounds->Min.Y
			});

			WrapInComment(
				NewMaterial,
				*Generator.GetBounds(),
				FString::Printf(TEXT("CustomOutputs: %s"), *MegaMaterial.CustomOutputsMaterial->GetName()));
		}
	}

	TVoxelMap<FVoxelMaterialRenderIndex, FVoxelMegaMaterialSwitchInputs> MaterialIndexToInputs;

	int32 PositionY = 0;
	for (const auto& It : IndexToSurfaceInfo)
	{
		const FVoxelMaterialRenderIndex RenderIndex = It.Key;
		UVoxelSurfaceTypeAsset& SurfaceType = *It.Value.SurfaceType;
		UMaterialInterface& MaterialInterface = *SurfaceType.Material;
		UMaterial& Material = *MaterialInterface.GetMaterial();

		VOXEL_SCOPE_COUNTER_FNAME(MaterialInterface.GetFName());

		CollectedWatchedMaterials(
			MaterialInterface,
			OutWatchedMaterials);

		const FString Prefix = "VOXELMATERIAL_" + FString::FromInt(RenderIndex.Index) + "_";

		FVoxelMaterialGenerator Generator(
			MegaMaterial,
			NewMaterial,
			Prefix,
			true,
			ShouldDuplicateFunction);

		FVoxelMegaMaterialSwitchInputs& Inputs = MaterialIndexToInputs.Add_EnsureNew(RenderIndex);
		{
			const TVoxelOptional<FMaterialAttributesInput> Input = Generator.CopyExpressions(Material);
			if (!ensure(Input))
			{
				return false;
			}

			Inputs.Attributes = FExpressionInput(*Input);
		}

		const UMaterialExpressionRuntimeVirtualTextureOutput* Output = INLINE_LAMBDA -> UMaterialExpressionRuntimeVirtualTextureOutput*
		{
			for (UMaterialExpression* Expression : FVoxelUtilities::GetMaterialExpressions(Material))
			{
				if (UMaterialExpressionRuntimeVirtualTextureOutput* Typed = Cast<UMaterialExpressionRuntimeVirtualTextureOutput>(Expression))
				{
					return Typed;
				}
			}

			return nullptr;
		};

		if (Output)
		{
			int32 AttributeIndex = 0;

			const auto TranslateFloat1 = [&](FExpressionInput Input, const float Constant)
			{
				if (Input.Expression)
				{
					Input.Expression = Generator.GetNewExpression(Input.Expression);
					ensure(Input.Expression);
					return Input;
				}

				UMaterialExpressionConstant& Expression = Generator.NewExpression<UMaterialExpressionConstant>();
				Expression.R = Constant;
				Expression.MaterialExpressionEditorX = Material.EditorX + 250;
				Expression.MaterialExpressionEditorY = Material.EditorY + (AttributeIndex++) * 50;

				FExpressionInput Result;
				Result.Expression = &Expression;
				return Result;
			};
			const auto TranslateFloat3 = [&](FExpressionInput Input, const FVector Constant)
			{
				if (Input.Expression)
				{
					Input.Expression = Generator.GetNewExpression(Input.Expression);
					ensure(Input.Expression);
					return Input;
				}

				UMaterialExpressionConstant3Vector& Expression = Generator.NewExpression<UMaterialExpressionConstant3Vector>();
				Expression.Constant = FLinearColor(Constant);
				Expression.MaterialExpressionEditorX = Material.EditorX + 250;
				Expression.MaterialExpressionEditorY = Material.EditorY + (AttributeIndex++) * 50;

				FExpressionInput Result;
				Result.Expression = &Expression;
				return Result;
			};
			const auto TranslateFloat4 = [&](FExpressionInput Input, const FLinearColor Constant)
			{
				if (Input.Expression)
				{
					Input.Expression = Generator.GetNewExpression(Input.Expression);
					ensure(Input.Expression);
					return Input;
				}

				UMaterialExpressionConstant4Vector& Expression = Generator.NewExpression<UMaterialExpressionConstant4Vector>();
				Expression.Constant = Constant;
				Expression.MaterialExpressionEditorX = Material.EditorX + 250;
				Expression.MaterialExpressionEditorY = Material.EditorY + (AttributeIndex++) * 50;

				FExpressionInput Result;
				Result.Expression = &Expression;
				return Result;
			};

			// Match UMaterialExpressionRuntimeVirtualTextureOutput::Compile

			Inputs.BaseColor = TranslateFloat3(Output->BaseColor, FVector(0.f, 0.f, 0.f));
			Inputs.Specular = TranslateFloat1(Output->Specular, 0.5f);
			Inputs.Roughness = TranslateFloat1(Output->Roughness, 0.5f);
			Inputs.Normal = TranslateFloat3(Output->Normal, FVector(0.f, 0.f, 1.f));
			Inputs.WorldHeight = TranslateFloat1(Output->WorldHeight, 0.f);
			Inputs.Opacity = TranslateFloat1(Output->Opacity, 1.f);
			Inputs.Mask = TranslateFloat1(Output->Mask, 1.f);
			Inputs.Displacement = TranslateFloat1(Output->Displacement, 0.f);
			Inputs.Mask4 = TranslateFloat4(Output->Mask4, FLinearColor(0.f, 0.f, 0.f, 0.f));
		}

		if (ShadingModels.Num() > 1 &&
			PrivateAccess::ShadingModel(Material) != MSM_FromMaterialExpression)
		{
			UMaterialExpressionShadingModel& ShadingModel = Generator.NewExpression<UMaterialExpressionShadingModel>();
			ShadingModel.ShadingModel = PrivateAccess::ShadingModel(Material);
			ShadingModel.MaterialExpressionEditorX = Material.EditorX + 250;
			ShadingModel.MaterialExpressionEditorY = Material.EditorY + 100;

			UMaterialExpressionSetMaterialAttributes& SetMaterialAttributes = Generator.NewExpression<UMaterialExpressionSetMaterialAttributes>();
			SetMaterialAttributes.AttributeSetTypes = { FMaterialAttributeDefinitionMap::GetID(MP_ShadingModel) };
			SetMaterialAttributes.Inputs[0] = Inputs.Attributes;
			SetMaterialAttributes.Inputs.Emplace_GetRef().Expression = &ShadingModel;
			SetMaterialAttributes.MaterialExpressionEditorX = Material.EditorX + 500;
			SetMaterialAttributes.MaterialExpressionEditorY = Material.EditorY;

			Inputs.Attributes.Expression = &SetMaterialAttributes;
		}

		DelayedCopyParameterValues.Add([&NewMaterialInstance, &MaterialInterface, Prefix](FMaterialInstanceParameterUpdateContext& InstanceUpdateContext)
		{
			FVoxelUtilities::CopyParameterValues(
				InstanceUpdateContext,
				NewMaterialInstance,
				MaterialInterface,
				Prefix);
		});

		Generator.ForeachExpression([&](UMaterialExpression& Expression)
		{
			UMaterialExpressionGetVoxelRandom* GetVoxelRandom = Cast<UMaterialExpressionGetVoxelRandom>(&Expression);
			if (!GetVoxelRandom)
			{
				return;
			}

			ensure(GetVoxelRandom->SurfaceTypeSeed.Seed.IsEmpty());
			GetVoxelRandom->SurfaceTypeSeed = SurfaceType.Seed;
		});

		if (const FVoxelOptionalIntBox2D Bounds = Generator.GetBounds())
		{
			Generator.MoveExpressions(FIntPoint
			{
				-700 - Bounds->Max.X,
				PositionY - Bounds->Min.Y
			});

			PositionY += Bounds->Size().Y + 1500;

			WrapInComment(
				NewMaterial,
				*Generator.GetBounds(),
				FString::Printf(TEXT("Material %d: %s"),
					RenderIndex.Index,
					*MaterialInterface.GetName()));
		}
	}

	NewMaterial.EditorX = 5700;
	NewMaterial.EditorY = PositionY / 2;

	{
		UMaterialExpressionVoxelMegaMaterialInternalSwitch& Switch = FVoxelUtilities::CreateMaterialExpression<UMaterialExpressionVoxelMegaMaterialInternalSwitch>(NewMaterial);
		Switch.MaterialExpressionEditorX = 5400;
		Switch.MaterialExpressionEditorY = PositionY / 2;
		Switch.Target = Target;
		Switch.bEnablePixelDepthOffset = MegaMaterial.bEnablePixelDepthOffset;
		Switch.bEnableSmoothBlends = INLINE_LAMBDA
		{
			if (!MegaMaterial.bEnableSmoothBlends)
			{
				return false;
			}

			for (const auto& It : IndexToSurfaceInfo)
			{
				if (It.Value.SurfaceType->BlendSmoothness > 0)
				{
					return true;
				}
			}

			// Disable if all smoothnesses are 0
			return false;
		};

		Switch.bEnableDitherNoiseTexture = MegaMaterial.bEnableDitherNoiseTexture && DitherNoiseTexture != nullptr;
		Switch.NumMetadatas = Metadatas.Num();

		// Default material
		Switch.IndexToInputs.Add(FVoxelMaterialRenderIndex(0), {});

		float MaxMagnitude = 0;
		for (const auto& It : IndexToSurfaceInfo)
		{
			const FVoxelMaterialRenderIndex RenderIndex = It.Key;
			UVoxelSurfaceTypeAsset& SurfaceType = *It.Value.SurfaceType;

			const UMaterialInterface& MaterialInterface = *SurfaceType.Material;

			MaxMagnitude = FMath::Max(MaxMagnitude, MaterialInterface.GetDisplacementScaling().Magnitude);

			FVoxelMegaMaterialSwitchInputs Inputs = MaterialIndexToInputs[RenderIndex];
			Inputs.bTangentSpaceNormal = MaterialInterface.GetMaterial()->bTangentSpaceNormal;
			Inputs.DisplacementScaling = MaterialInterface.GetDisplacementScaling();

			ensure(!Switch.IndexToInputs.Contains(RenderIndex));
			Switch.IndexToInputs.Add(RenderIndex, Inputs);
		}

		NewMaterial.DisplacementScaling.Magnitude = MaxMagnitude;
		NewMaterial.DisplacementScaling.Center = 0.5f;

		NewMaterial.GetEditorOnlyData()->MaterialAttributes.Expression = &Switch;

		UMaterialExpressionRuntimeVirtualTextureOutput& Output = FVoxelUtilities::CreateMaterialExpression<UMaterialExpressionRuntimeVirtualTextureOutput>(NewMaterial);
		Output.MaterialExpressionEditorX = 5700;
		Output.MaterialExpressionEditorY = PositionY / 2 + 70;
		Output.BaseColor.Connect(1, &Switch);
		Output.Specular.Connect(2, &Switch);
		Output.Roughness.Connect(3, &Switch);
		Output.Normal.Connect(4, &Switch);
		Output.WorldHeight.Connect(5, &Switch);
		Output.Opacity.Connect(6, &Switch);
		Output.Mask.Connect(7, &Switch);
		Output.Displacement.Connect(8, &Switch);
		Output.Mask4.Connect(9, &Switch);
	}

	if (!AddAttributePostProcess(MegaMaterial, NewMaterial))
	{
		return false;
	}

	for (UMaterialExpression* Expression : FVoxelUtilities::GetMaterialExpressions_Recursive(NewMaterial))
	{
		if (UMaterialExpressionVoxelMegaMaterialSwitch* Switch = Cast<UMaterialExpressionVoxelMegaMaterialSwitch>(Expression))
		{
			if (!ensure(Expression->GetTypedOuter<UMaterial>() == &NewMaterial))
			{
				continue;
			}

			ensure(!Switch->bIsMegaMaterial);
			Switch->bIsMegaMaterial = true;

			Switch->Default = {};
		}

		if (UMaterialExpressionGetVoxelMetadata* GetVoxelMetadata = Cast<UMaterialExpressionGetVoxelMetadata>(Expression))
		{
			if (!ensure(Expression->GetTypedOuter<UMaterial>() == &NewMaterial))
			{
				continue;
			}

			ensure(GetVoxelMetadata->MetadataIndex == -1);

			if (GetVoxelMetadata->Metadata)
			{
				GetVoxelMetadata->MetadataIndex = Metadatas.Find(GetVoxelMetadata->Metadata);
				ensure(GetVoxelMetadata->MetadataIndex != -1);
			}
		}
	}

	NewMaterial.PostEditChange();

	{
		FMaterialInstanceParameterUpdateContext InstanceUpdateContext(&NewMaterialInstance, EMaterialInstanceClearParameterFlag::All);

		for (const TFunction<void(FMaterialInstanceParameterUpdateContext&)>& CopyParameterValues : DelayedCopyParameterValues)
		{
			CopyParameterValues(InstanceUpdateContext);
		}

		ApplyBlendSmoothness(
			IndexToSurfaceInfo,
			NewMaterial,
			NewMaterialInstance);

		TGuardValue<bool> Guard(GIsEditor, true);
		NewMaterialInstance.SetTextureParameterValueEditorOnly(FName("VOXEL_PixelNoiseTexture"), DitherNoiseTexture);
	}

	// Reduce SRVs
	FVoxelUtilities::MergeIdenticalTextureParameters(NewMaterial, NewMaterialInstance);

	NewMaterial.PostEditChange();

	return true;
}

bool FVoxelMegaMaterialGenerator::GenerateMaterial(
	const UVoxelMegaMaterial& MegaMaterial,
	const UVoxelSurfaceTypeAsset& SurfaceType,
	UMaterial& NewMaterial,
	UMaterialInstanceConstant& NewMaterialInstance,
	const TConstVoxelArrayView<TObjectPtr<UVoxelMetadata>> Metadatas,
	const bool bIsDummyMaterial)
{
	VOXEL_FUNCTION_COUNTER();
	check(NewMaterialInstance.Parent == &NewMaterial);

	const UMaterialInterface& MaterialInterface = *SurfaceType.Material;

	if (!ensure(MaterialInterface.GetMaterial()))
	{
		return false;
	}
	const UMaterial& Material = *MaterialInterface.GetMaterial();

	TUniquePtr<FMaterialUpdateContext> UpdateContext;
	if (!bIsDummyMaterial)
	{
		UpdateContext = MakeUnique<FMaterialUpdateContext>();
		UpdateContext->AddMaterial(&NewMaterial);
		UpdateContext->AddMaterialInstance(&NewMaterialInstance);
	}

	for (FProperty& Property : GetClassProperties<UMaterial>())
	{
		if (!Property.HasAllPropertyFlags(CPF_Edit) ||
			Property.HasAnyPropertyFlags(CPF_InstancedReference))
		{
			continue;
		}

		if (Property.GetName().StartsWith("bUsedWith"))
		{
			CastFieldChecked<FBoolProperty>(Property).SetPropertyValue_InContainer(&NewMaterial, false);
			continue;
		}

		Property.CopyCompleteValue_InContainer(&NewMaterial, &Material);
	}

#if VOXEL_ENGINE_VERSION >= 506
	// Otherwise "Cooking a material resource that doesn't have a valid ShaderMap"
	NewMaterial.bUsedWithStaticMesh = true;
#endif

	NewMaterial.bUsedWithNanite = true;
	NewMaterial.bEnableTessellation = true;

	if (MegaMaterial.bSetHasPixelAnimation)
	{
		NewMaterial.bHasPixelAnimation = true;
	}

	if (const UMaterialInstance* Instance = Cast<UMaterialInstance>(&MaterialInterface))
	{
		NewMaterialInstance.BasePropertyOverrides = Instance->BasePropertyOverrides;
	}

	NewMaterial.bUseMaterialAttributes = true;

	FVoxelUtilities::ClearMaterialExpressions(NewMaterial);

	TVoxelArray<TFunction<void(FMaterialInstanceParameterUpdateContext&)>> DelayedCopyParameterValues;

	FVoxelMaterialGenerator Generator(
		MegaMaterial,
		NewMaterial,
		{},
		false,
		ShouldDuplicateFunction);

	const TVoxelOptional<FMaterialAttributesInput> Input = Generator.CopyExpressions(Material);
	if (!ensure(Input))
	{
		return false;
	}

	Generator.ForeachExpression([&](UMaterialExpression& Expression)
	{
		UMaterialExpressionGetVoxelRandom* GetVoxelRandom = Cast<UMaterialExpressionGetVoxelRandom>(&Expression);
		if (!GetVoxelRandom)
		{
			return;
		}

		ensure(GetVoxelRandom->SurfaceTypeSeed.Seed.IsEmpty());
		GetVoxelRandom->SurfaceTypeSeed = SurfaceType.Seed;
	});

	NewMaterial.EditorX = Material.EditorX + 500;
	NewMaterial.EditorY = Material.EditorY;

	{
		UMaterialExpressionVoxelNaniteMaterialHook& Hook = FVoxelUtilities::CreateMaterialExpression<UMaterialExpressionVoxelNaniteMaterialHook>(NewMaterial);
		Hook.MaterialExpressionEditorX = Material.EditorX + 200;
		Hook.MaterialExpressionEditorY = Material.EditorY;
		Hook.NumMetadatas = Metadatas.Num();
		Hook.Input = FExpressionInput(*Input);

		NewMaterial.GetEditorOnlyData()->MaterialAttributes.Expression = &Hook;
	}

	if (!AddAttributePostProcess(MegaMaterial, NewMaterial))
	{
		return false;
	}

	for (UMaterialExpression* Expression : FVoxelUtilities::GetMaterialExpressions_Recursive(NewMaterial))
	{
		if (UMaterialExpressionGetVoxelMetadata* GetVoxelMetadata = Cast<UMaterialExpressionGetVoxelMetadata>(Expression))
		{
			if (!ensure(Expression->GetTypedOuter<UMaterial>() == &NewMaterial))
			{
				continue;
			}

			ensure(GetVoxelMetadata->MetadataIndex == -1);

			if (GetVoxelMetadata->Metadata)
			{
				GetVoxelMetadata->MetadataIndex = Metadatas.Find(GetVoxelMetadata->Metadata);
				ensure(GetVoxelMetadata->MetadataIndex != -1);
			}
		}
	}

	NewMaterial.PostEditChange();

	FMaterialInstanceParameterUpdateContext InstanceUpdateContext(&NewMaterialInstance, EMaterialInstanceClearParameterFlag::All);

	FVoxelUtilities::CopyParameterValues(
		InstanceUpdateContext,
		NewMaterialInstance,
		MaterialInterface,
		{});

	return true;
}

bool FVoxelMegaMaterialGenerator::ApplyBlendSmoothness(
	const TMap<FVoxelMaterialRenderIndex, FVoxelMegaMaterialSurfaceInfo>& IndexToSurfaceInfo,
	const UMaterial& Material,
	UMaterialInstanceConstant& MaterialInstance)
{
	VOXEL_FUNCTION_COUNTER();

	bool bChanged = false;

	for (const auto& It : IndexToSurfaceInfo)
	{
		const FVoxelMaterialRenderIndex RenderIndex = It.Key;
		const UVoxelSurfaceTypeAsset& SurfaceType = *It.Value.SurfaceType;

		// Apply displacement scaling, but keep it relative to output magnitude just like displacement in FVoxelMegaMaterialSwitch
		const float Scale =
			SurfaceType.Material->GetDisplacementScaling().Magnitude /
			Material.GetDisplacementScaling().Magnitude;

		const float NewValue = FMath::Max(SurfaceType.BlendSmoothness, 0.f) * Scale;

		const FName Name = FName(FString::Printf(TEXT("VOXEL_BlendSmoothness_%d"), RenderIndex.Index));

		float Value = 0.f;
		if (MaterialInstance.GetScalarParameterValue(Name, Value) &&
			Value == NewValue)
		{
			continue;
		}

		{
			TGuardValue<bool> Guard(GIsEditor, true);
			MaterialInstance.SetScalarParameterValueEditorOnly(Name, NewValue);
		}

		bChanged = true;
	}

	return bChanged;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelArray<UVoxelMetadata*> FVoxelMegaMaterialGenerator::GetUsedMetadatas(const UMaterial& Material)
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelSet<UVoxelMetadata*> Metadatas;

	for (UMaterialExpression* Expression : FVoxelUtilities::GetMaterialExpressions_Recursive(Material))
	{
		const UMaterialExpressionGetVoxelMetadata* GetVoxelMetadata = Cast<UMaterialExpressionGetVoxelMetadata>(Expression);
		if (!GetVoxelMetadata)
		{
			continue;
		}
		ensure(GetVoxelMetadata->MetadataIndex == -1);

		if (!GetVoxelMetadata->Metadata)
		{
			continue;
		}

		Metadatas.Add(GetVoxelMetadata->Metadata);
	}

	return Metadatas.Array();
}

TVoxelArray<UVoxelMetadata*> FVoxelMegaMaterialGenerator::GetUsedMetadatas(const UMaterialFunction& MaterialFunction)
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelSet<UVoxelMetadata*> Metadatas;

	for (UMaterialExpression* Expression : FVoxelUtilities::GetMaterialExpressions_Recursive(MaterialFunction))
	{
		const UMaterialExpressionGetVoxelMetadata* GetVoxelMetadata = Cast<UMaterialExpressionGetVoxelMetadata>(Expression);
		if (!GetVoxelMetadata)
		{
			continue;
		}
		ensure(GetVoxelMetadata->MetadataIndex == -1);

		if (!GetVoxelMetadata->Metadata)
		{
			continue;
		}

		Metadatas.Add(GetVoxelMetadata->Metadata);
	}

	return Metadatas.Array();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelMegaMaterialGenerator::AddAttributePostProcess(
	const UVoxelMegaMaterial& MegaMaterial,
	UMaterial& Material)
{
	if (!MegaMaterial.AttributePostProcess)
	{
		return true;
	}

	{
		TArray<FFunctionExpressionInput> Inputs;
		TArray<FFunctionExpressionOutput> Outputs;
		MegaMaterial.AttributePostProcess->GetInputsAndOutputs(Inputs, Outputs);

		if (Inputs.Num() != 1 ||
			Outputs.Num() != 1)
		{
			VOXEL_MESSAGE(Error, "{0}: MegaMaterial AttributePostProcess should have exactly one input and one output, both of the type MaterialAttributes", MegaMaterial);
			return false;
		}
	}

	FVoxelMaterialGenerator Generator(
		MegaMaterial,
		Material,
		{},
		false,
		ShouldDuplicateFunction);

	UMaterialFunction* NewFunction = Generator.DuplicateFunctionIfNeeded(*MegaMaterial.AttributePostProcess);
	if (!ensureVoxelSlow(NewFunction))
	{
		return false;
	}

	UMaterialExpressionMaterialFunctionCall& FunctionCall = FVoxelUtilities::CreateMaterialExpression<UMaterialExpressionMaterialFunctionCall>(Material);
	FunctionCall.SetMaterialFunction(NewFunction);
	FunctionCall.MaterialExpressionEditorX = Material.EditorX;
	FunctionCall.MaterialExpressionEditorY = Material.EditorY;
	FunctionCall.FunctionInputs[0].Input = FExpressionInput(Material.GetEditorOnlyData()->MaterialAttributes);

	Material.EditorX += 200;

	Material.GetEditorOnlyData()->MaterialAttributes.Expression = &FunctionCall;

	return true;
}

void FVoxelMegaMaterialGenerator::CollectedWatchedMaterials(
	UMaterialInterface& Material,
	TVoxelSet<TVoxelObjectPtr<UObject>>& OutWatchedMaterials)
{
	UMaterialInterface* Value = &Material;
	while (Value)
	{
		OutWatchedMaterials.Add(Value);

		const UMaterialInstance* Instance = Cast<UMaterialInstance>(Value);
		if (!Instance)
		{
			break;
		}

		Value = Instance->Parent;
	}
}

void FVoxelMegaMaterialGenerator::WrapInComment(
	UMaterial& Material,
	const FVoxelIntBox2D& Bounds,
	const FString& Comment)
{
	const FVoxelIntBox2D FinalBounds = Bounds.Extend(100);

	UMaterialExpressionComment& Expression = FVoxelUtilities::CreateMaterialExpression<UMaterialExpressionComment>(Material);
	Expression.Text = Comment;
	Expression.SizeX = FinalBounds.Size().X + 200;
	Expression.SizeY = FinalBounds.Size().Y + 500;
	Expression.MaterialExpressionEditorX = FinalBounds.Min.X;
	Expression.MaterialExpressionEditorY = FinalBounds.Min.Y;
}

bool FVoxelMegaMaterialGenerator::ShouldDuplicateFunction(const UMaterialExpression& Expression)
{
	return
		Expression.IsA<UMaterialExpressionGetVoxelRandom>() ||
		Expression.IsA<UMaterialExpressionGetVoxelMetadata>() ||
		Expression.IsA<UMaterialExpressionVoxelMegaMaterialSwitch>();
}
#endif