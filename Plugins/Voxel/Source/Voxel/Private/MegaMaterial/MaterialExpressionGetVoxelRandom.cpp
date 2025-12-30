// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "MegaMaterial/MaterialExpressionGetVoxelRandom.h"
#include "Materials/MaterialExpressionCustom.h"
#include "VoxelRuntimePinValue.h"
#include "MaterialCompiler.h"
#include "Engine/Texture2D.h"

UMaterialExpressionGetVoxelRandom::UMaterialExpressionGetVoxelRandom()
{
	MenuCategories.Add(INVTEXT("Voxel Plugin"));

	Outputs.Reset();
	Outputs.Add({ "Value" });
}

void UMaterialExpressionGetVoxelRandom::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	SerializeVoxelVersion(Ar);
}

#if WITH_EDITOR
uint32 UMaterialExpressionGetVoxelRandom::GetOutputType(int32 OutputIndex)
{
	return MCT_Float;
}

int32 UMaterialExpressionGetVoxelRandom::Compile(FMaterialCompiler* Compiler, int32 OutputIndex)
{
	uint64 Hash = FVoxelUtilities::MurmurHash(Seed.GetSeed());
	Hash = FVoxelUtilities::MurmurHashMulti(Hash, SurfaceTypeSeed.GetSeed());

	const FRandomStream Stream(Hash);
	const float Value = Stream.FRandRange(Min, Max);

	return Compiler->Constant(Value);
}

void UMaterialExpressionGetVoxelRandom::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add("Get Voxel Random [" + LexToSanitizedString(Min) + ", " + LexToSanitizedString(Max) + "]");
}
#endif