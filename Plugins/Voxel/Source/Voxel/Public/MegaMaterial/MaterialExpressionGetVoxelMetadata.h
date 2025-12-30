// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMetadata.h"
#include "Materials/MaterialExpression.h"
#include "MaterialExpressionGetVoxelMetadata.generated.h"

UCLASS(DisplayName = "Get Voxel Metadata")
class VOXEL_API UMaterialExpressionGetVoxelMetadata : public UMaterialExpression
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	TObjectPtr<UVoxelMetadata> Metadata;

	UPROPERTY()
	int32 MetadataIndex = -1;

public:
	UMaterialExpressionGetVoxelMetadata();

	//~ Begin UMaterialExpression Interface
	virtual void Serialize(FArchive& Ar) override;
	virtual UObject* GetReferencedTexture() const override;
#if WITH_EDITOR
	virtual uint32 GetOutputType(int32 OutputIndex) override;
	virtual int32 Compile(FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
#endif
	//~ End UMaterialExpression Interface
};