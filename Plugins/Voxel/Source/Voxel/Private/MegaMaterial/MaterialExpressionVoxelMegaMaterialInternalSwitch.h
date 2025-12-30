// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Materials/MaterialExpression.h"
#include "MegaMaterial/VoxelRenderMaterial.h"
#include "MegaMaterial/VoxelMegaMaterialTarget.h"
#include "MaterialExpressionVoxelMegaMaterialInternalSwitch.generated.h"

USTRUCT()
struct FVoxelMegaMaterialSwitchInputs
{
	GENERATED_BODY()

	UPROPERTY()
	FExpressionInput Attributes;

	/** Input for Base Color to output to virtual texture. */
	UPROPERTY()
	FExpressionInput BaseColor;

	/** Input for Specular to output to virtual texture. */
	UPROPERTY()
	FExpressionInput Specular;

	/** Input for Roughness to output to virtual texture. */
	UPROPERTY()
	FExpressionInput Roughness;

	/** Input for Surface Normal to output to virtual texture. */
	UPROPERTY()
	FExpressionInput Normal;

	/** Input for World Height to output to virtual texture. */
	UPROPERTY()
	FExpressionInput WorldHeight;

	/** Input for Opacity value used for blending to virtual texture. */
	UPROPERTY()
	FExpressionInput Opacity;

	/** Input for Mask to output to virtual texture. */
	UPROPERTY()
	FExpressionInput Mask;

	/** Input for World Height to output to virtual texture. */
	UPROPERTY()
	FExpressionInput Displacement;

	/** Input for Mask to output to virtual texture. */
	UPROPERTY()
	FExpressionInput Mask4;

	UPROPERTY()
	bool bTangentSpaceNormal = false;

	UPROPERTY()
	FDisplacementScaling DisplacementScaling;
};

UCLASS(meta = (Private))
class UMaterialExpressionVoxelMegaMaterialInternalSwitch : public UMaterialExpression
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "Config")
	EVoxelMegaMaterialTarget Target = {};

	UPROPERTY(VisibleAnywhere, Category = "Config")
	bool bEnablePixelDepthOffset = false;

	UPROPERTY(VisibleAnywhere, Category = "Config")
	bool bEnableSmoothBlends = false;

	UPROPERTY(VisibleAnywhere, Category = "Config")
	bool bEnableDitherNoiseTexture = false;

	UPROPERTY(VisibleAnywhere, Category = "Config")
	int32 NumMetadatas = 0;

	UPROPERTY()
	TMap<FVoxelMaterialRenderIndex, FVoxelMegaMaterialSwitchInputs> IndexToInputs;

	UMaterialExpressionVoxelMegaMaterialInternalSwitch();

	//~ Begin UMaterialExpression Interface
	virtual UObject* GetReferencedTexture() const override;
	virtual bool CanReferenceTexture() const override { return true; }

#if WITH_EDITOR
	virtual uint32 GetOutputType(int32 OutputIndex) override;
	virtual bool IsResultMaterialAttributes(int32 OutputIndex) override;
	virtual int32 Compile(FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	virtual FName GetInputName(int32 InputIndex) const override;
	virtual FExpressionInput* GetInput(int32 InputIndex) override;
	virtual TArrayView<FExpressionInput*> GetInputsView() override;
#endif
	//~ End UMaterialExpression Interface
};