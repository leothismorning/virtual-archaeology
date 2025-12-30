// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "MaterialValueType.h"
#include "Materials/MaterialExpression.h"
#include "MaterialExpressionGetVoxelMaterialInfo.generated.h"

UCLASS(DisplayName = "Get Voxel Material Info")
class VOXEL_API UMaterialExpressionGetVoxelMaterialInfo : public UMaterialExpression
{
	GENERATED_BODY()

public:
	UMaterialExpressionGetVoxelMaterialInfo();

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