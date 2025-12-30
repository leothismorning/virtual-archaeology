// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "MegaMaterial/MaterialExpressionGetVoxelMetadata.h"
#include "Materials/MaterialExpressionCustom.h"
#include "VoxelRuntimePinValue.h"
#include "MaterialCompiler.h"
#include "Engine/Texture2D.h"

UMaterialExpressionGetVoxelMetadata::UMaterialExpressionGetVoxelMetadata()
{
	MenuCategories.Add(INVTEXT("Voxel Plugin"));

	Outputs.Reset();
	Outputs.Add({ "Value" });
}

void UMaterialExpressionGetVoxelMetadata::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	SerializeVoxelVersion(Ar);
}

UObject* UMaterialExpressionGetVoxelMetadata::GetReferencedTexture() const
{
	return FVoxelTextureUtilities::GetDefaultTexture2D();
}

#if WITH_EDITOR
uint32 UMaterialExpressionGetVoxelMetadata::GetOutputType(int32 OutputIndex)
{
	if (!Metadata)
	{
		return MCT_Float;
	}

	const TVoxelOptional<EVoxelMetadataMaterialType> Type = Metadata->GetMaterialType();
	if (!Type)
	{
		return MCT_Float;
	}

	return FVoxelMetadataMaterialType::GetMaterialValueType(Type.GetValue());
}

int32 UMaterialExpressionGetVoxelMetadata::Compile(FMaterialCompiler* Compiler, int32 OutputIndex)
{
	VOXEL_FUNCTION_COUNTER();

	UTexture2D* DefaultTexture = FVoxelTextureUtilities::GetDefaultTexture2D();
	if (!ensure(DefaultTexture))
	{
		return -1;
	}

	const FVoxelMetadataRef MetadataRef(Metadata);
	if (!MetadataRef)
	{
		return Compiler->Error(TEXT("Missing Metadata"));
	}

	const TVoxelOptional<EVoxelMetadataMaterialType> OptionalMaterialType = MetadataRef.GetMaterialType();
	if (!OptionalMaterialType)
	{
		return Compiler->Error(TEXT("Metadata cannot be used in a material"));
	}
	const EVoxelMetadataMaterialType MaterialType = OptionalMaterialType.GetValue();

	if (MetadataIndex == -1)
	{
		return FVoxelMetadataMaterialType::Constant(
			*Compiler,
			MetadataRef,
			MaterialType,
			MetadataRef.GetDefaultValue());
	}
	ensure(MetadataIndex >= 0);

	UMaterialExpressionCustom* Custom = NewObject<UMaterialExpressionCustom>();
	Custom->Code.Reset();
	Custom->Inputs.Reset();
	Custom->OutputType = FVoxelMetadataMaterialType::GetCustomMaterialOutputType(MaterialType);
	Custom->IncludeFilePaths.Add("/Plugin/Voxel/Common.ush");

	TArray<int32> Inputs;

	{
		Inputs.Add(Compiler->TextureParameter(
			"VOXEL_AttributesIndices_Texture",
			DefaultTexture,
			SAMPLERTYPE_Color));

		Custom->Inputs.Add({ "AttributesIndices_Texture" });
	}

	{
		Inputs.Add(Compiler->ScalarParameter("VOXEL_AttributesIndices_TextureSizeLog2", 0.f));
		Custom->Inputs.Add({ "AttributesIndices_TextureSizeLog2" });
	}

	{
		Inputs.Add(Compiler->TextureParameter(
			FName("VOXEL_Metadata_Texture", MetadataIndex + 1),
			DefaultTexture,
			SAMPLERTYPE_Color));
		Custom->Inputs.Add({ "Metadata_Texture" });
	}

	{
		Inputs.Add(Compiler->ScalarParameter(FName("VOXEL_Metadata_TextureSizeLog2", MetadataIndex + 1), 0.f));
		Custom->Inputs.Add({ "Metadata_TextureSizeLog2" });
	}

	FString GetMetadataCode;

	if (MaterialType == EVoxelMetadataMaterialType::Normal)
	{
		Custom->Code = R"(
#if MATERIAL_VERTEX_PARAMETERS_VOXEL_VERSION == 5 && MATERIAL_PIXEL_PARAMETERS_VOXEL_VERSION == 7
#if VERTEX_SHADER
	const int Index = asint(AttributesIndices_Texture[GetTextureIndex_Log2(Parameters.Voxel_AttributesIndicesIndex + 1 + METADATA_INDEX, AttributesIndices_TextureSizeLog2)].r);
	return Index == -1 ? DEFAULT_METADATA : GET_METADATA;
#else
	const int Index0 = asint(AttributesIndices_Texture[GetTextureIndex_Log2(Parameters.Voxel_AttributesIndicesIndices[0] + 1 + METADATA_INDEX, AttributesIndices_TextureSizeLog2)].r);
	const int Index1 = asint(AttributesIndices_Texture[GetTextureIndex_Log2(Parameters.Voxel_AttributesIndicesIndices[1] + 1 + METADATA_INDEX, AttributesIndices_TextureSizeLog2)].r);
	const int Index2 = asint(AttributesIndices_Texture[GetTextureIndex_Log2(Parameters.Voxel_AttributesIndicesIndices[2] + 1 + METADATA_INDEX, AttributesIndices_TextureSizeLog2)].r);

	const float3 Value0 = Index0 == -1 ? DEFAULT_METADATA : GET_METADATA_0;
	const float3 Value1 = Index1 == -1 ? DEFAULT_METADATA : GET_METADATA_1;
	const float3 Value2 = Index2 == -1 ? DEFAULT_METADATA : GET_METADATA_2;

	return normalize(
		Value0 * Parameters.Voxel_Barycentrics[0] +
		Value1 * Parameters.Voxel_Barycentrics[1] +
		Value2 * Parameters.Voxel_Barycentrics[2]);
#endif
#else
	return 0;
#endif)";

		GetMetadataCode = "OctahedronToUnitVector(2.f * asfloat(Metadata_Texture[GetTextureIndex_Log2(IndexX, Metadata_TextureSizeLog2)].rg) - 1.f)";
	}
	else
	{
		Custom->Code = R"(
#if MATERIAL_VERTEX_PARAMETERS_VOXEL_VERSION == 5 && MATERIAL_PIXEL_PARAMETERS_VOXEL_VERSION == 7
#if VERTEX_SHADER
	const int Index = asint(AttributesIndices_Texture[GetTextureIndex_Log2(Parameters.Voxel_AttributesIndicesIndex + 1 + METADATA_INDEX, AttributesIndices_TextureSizeLog2)].r);
	return Index == -1 ? DEFAULT_METADATA : GET_METADATA;
#else
	const int Index0 = asint(AttributesIndices_Texture[GetTextureIndex_Log2(Parameters.Voxel_AttributesIndicesIndices[0] + 1 + METADATA_INDEX, AttributesIndices_TextureSizeLog2)].r);
	const int Index1 = asint(AttributesIndices_Texture[GetTextureIndex_Log2(Parameters.Voxel_AttributesIndicesIndices[1] + 1 + METADATA_INDEX, AttributesIndices_TextureSizeLog2)].r);
	const int Index2 = asint(AttributesIndices_Texture[GetTextureIndex_Log2(Parameters.Voxel_AttributesIndicesIndices[2] + 1 + METADATA_INDEX, AttributesIndices_TextureSizeLog2)].r);

	const TYPE Value0 = Index0 == -1 ? DEFAULT_METADATA : GET_METADATA_0;
	const TYPE Value1 = Index1 == -1 ? DEFAULT_METADATA : GET_METADATA_1;
	const TYPE Value2 = Index2 == -1 ? DEFAULT_METADATA : GET_METADATA_2;

	if (INTERPOLATE)
	{
		return
			Value0 * Parameters.Voxel_Barycentrics[0] +
			Value1 * Parameters.Voxel_Barycentrics[1] +
			Value2 * Parameters.Voxel_Barycentrics[2];
	}
	else
	{
		if (Parameters.Voxel_Barycentrics[0] >= Parameters.Voxel_Barycentrics[1] &&
			Parameters.Voxel_Barycentrics[0] >= Parameters.Voxel_Barycentrics[2])
		{
			return Value0;
		}

		if (Parameters.Voxel_Barycentrics[1] >= Parameters.Voxel_Barycentrics[2])
		{
			return Value1;
		}

		return Value2;
	}
#endif
#else
	return 0;
#endif)";

		{
			const FString Code = INLINE_LAMBDA -> FString
			{
				switch (MaterialType)
				{
				default: ensure(false);
				case EVoxelMetadataMaterialType::Float1:
				case EVoxelMetadataMaterialType::Float2:
				case EVoxelMetadataMaterialType::Float3:
				case EVoxelMetadataMaterialType::Float4:
				{
					return "true";
				}
				case EVoxelMetadataMaterialType::Int1:
				case EVoxelMetadataMaterialType::Int2:
				case EVoxelMetadataMaterialType::Int3:
				case EVoxelMetadataMaterialType::Int4:
				{
					return "false";
				}
				}
			};

			Custom->Code.ReplaceInline(
				TEXT("INTERPOLATE"),
				*Code,
				ESearchCase::CaseSensitive);
		}

		{
			const FString Code = INLINE_LAMBDA
			{
				switch (MaterialType)
				{
				default: ensure(false);
				case EVoxelMetadataMaterialType::Float1: return "float";
				case EVoxelMetadataMaterialType::Float2: return "float2";
				case EVoxelMetadataMaterialType::Float3: return "float3";
				case EVoxelMetadataMaterialType::Float4: return "float4";
				case EVoxelMetadataMaterialType::Int1: return "int";
				case EVoxelMetadataMaterialType::Int2: return "int2";
				case EVoxelMetadataMaterialType::Int3: return "int3";
				case EVoxelMetadataMaterialType::Int4: return "int4";
				}
			};

			Custom->Code.ReplaceInline(
				TEXT("TYPE"),
				*Code,
				ESearchCase::CaseSensitive);
		}

		GetMetadataCode = INLINE_LAMBDA -> FString
		{
			switch (MaterialType)
			{
			default: ensure(false);
			case EVoxelMetadataMaterialType::Float1: return "asfloat(Metadata_Texture[GetTextureIndex_Log2(IndexX, Metadata_TextureSizeLog2)].r)";
			case EVoxelMetadataMaterialType::Float2: return "asfloat(Metadata_Texture[GetTextureIndex_Log2(IndexX, Metadata_TextureSizeLog2)].rg)";
			case EVoxelMetadataMaterialType::Float3: return "asfloat(Metadata_Texture[GetTextureIndex_Log2(IndexX, Metadata_TextureSizeLog2)].rgb)";
			case EVoxelMetadataMaterialType::Float4: return "asfloat(Metadata_Texture[GetTextureIndex_Log2(IndexX, Metadata_TextureSizeLog2)].rgba)";
			case EVoxelMetadataMaterialType::Int1: return "float(asint(Metadata_Texture[GetTextureIndex_Log2(IndexX, Metadata_TextureSizeLog2)].r))";
			case EVoxelMetadataMaterialType::Int2: return "float2(asint(Metadata_Texture[GetTextureIndex_Log2(IndexX, Metadata_TextureSizeLog2)].rg))";
			case EVoxelMetadataMaterialType::Int3: return "float3(asint(Metadata_Texture[GetTextureIndex_Log2(IndexX, Metadata_TextureSizeLog2)].rgb))";
			case EVoxelMetadataMaterialType::Int4: return "float4(asint(Metadata_Texture[GetTextureIndex_Log2(IndexX, Metadata_TextureSizeLog2)].rgba))";
			}
		};
	}

	Custom->Code.ReplaceInline(TEXT("VERTEX_SHADER"), Compiler->GetCurrentShaderFrequency() == SF_Vertex
		? TEXT("1")
		: TEXT("0"));

	Custom->Code.ReplaceInline(
		TEXT("METADATA_INDEX"),
		*FString::FromInt(MetadataIndex),
		ESearchCase::CaseSensitive);

	{
		const FString Code = FVoxelMetadataMaterialType::Constant(
			MetadataRef,
			MaterialType,
			MetadataRef.GetDefaultValue());

		Custom->Code.ReplaceInline(
			TEXT("DEFAULT_METADATA"),
			*Code,
			ESearchCase::CaseSensitive);
	}

	Custom->Code.ReplaceInline(
		TEXT("GET_METADATA_0"),
		*GetMetadataCode.Replace(TEXT("IndexX"), TEXT("Index0"), ESearchCase::CaseSensitive),
		ESearchCase::CaseSensitive);

	Custom->Code.ReplaceInline(
		TEXT("GET_METADATA_1"),
		*GetMetadataCode.Replace(TEXT("IndexX"), TEXT("Index1"), ESearchCase::CaseSensitive),
		ESearchCase::CaseSensitive);

	Custom->Code.ReplaceInline(
		TEXT("GET_METADATA_2"),
		*GetMetadataCode.Replace(TEXT("IndexX"), TEXT("Index2"), ESearchCase::CaseSensitive),
		ESearchCase::CaseSensitive);

	Custom->Code.ReplaceInline(
		TEXT("GET_METADATA"),
		*GetMetadataCode.Replace(TEXT("IndexX"), TEXT("Index"), ESearchCase::CaseSensitive),
		ESearchCase::CaseSensitive);

	if (!ensureVoxelSlow(!Inputs.Contains(-1)))
	{
		return Compiler->Errorf(TEXT("Invalid texture index"));
	}

	return Compiler->CustomExpression(Custom, 0, Inputs);
}

void UMaterialExpressionGetVoxelMetadata::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add("Get Voxel Metadata: " + (Metadata ? Metadata->GetName() : "null"));
}
#endif