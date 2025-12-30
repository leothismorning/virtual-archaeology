// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "MaterialValueType.h"
#include "Materials/MaterialExpression.h"
#include "MaterialExpressionVoxelLayer.generated.h"

UCLASS(DisplayName = "Voxel Layer")
class VOXEL_API UMaterialExpressionVoxelLayer : public UMaterialExpression
{
	GENERATED_BODY()

public:
	UMaterialExpressionVoxelLayer();

	//~ Begin UMaterialExpression Interface
	virtual void Serialize(FArchive& Ar) override;
#if WITH_EDITOR
	virtual uint32 GetOutputType(int32 OutputIndex) override
	{
		return MCT_Float;
	}
	virtual int32 Compile(FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
#endif
	//~ End UMaterialExpression Interface
};