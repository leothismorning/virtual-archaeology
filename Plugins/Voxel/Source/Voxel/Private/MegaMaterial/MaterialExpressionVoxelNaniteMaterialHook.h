// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "MaterialValueType.h"
#include "Materials/MaterialExpression.h"
#include "MaterialExpressionVoxelNaniteMaterialHook.generated.h"

UCLASS(meta = (Private))
class UMaterialExpressionVoxelNaniteMaterialHook : public UMaterialExpression
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "Config")
	int32 NumMetadatas = 0;

	UPROPERTY()
	FExpressionInput Input;

	//~ Begin UMaterialExpression Interface
	virtual UObject* GetReferencedTexture() const override;
	virtual bool CanReferenceTexture() const override { return true; }

#if WITH_EDITOR
	virtual uint32 GetOutputType(int32 OutputIndex) override
	{
		return MCT_MaterialAttributes;
	}
	virtual bool IsResultMaterialAttributes(int32 OutputIndex) override
	{
		return true;
	}
	virtual int32 Compile(FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	virtual FExpressionInput* GetInput(int32 InputIndex) override;
	virtual TArrayView<FExpressionInput*> GetInputsView() override;
#endif
	//~ End UMaterialExpression Interface
};