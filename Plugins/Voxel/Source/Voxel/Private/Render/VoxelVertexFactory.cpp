// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelVertexFactory.h"
#include "VoxelBufferPool.h"
#include "VoxelMaterialUsage.h"
#include "MegaMaterial/VoxelMegaMaterialProxy.h"
#include "MegaMaterial/VoxelMegaMaterialRenderUtilities.h"
#include "MaterialDomain.h"
#include "MeshMaterialShader.h"
#include "MeshDrawShaderBindings.h"

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FVoxelVertexFactoryGlobalParameters, "VoxelVertexFactory");

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelVertexFactoryShaderParameters::GetElementShaderBindings(
	const FSceneInterface* Scene,
	const FSceneView* View,
	const FMeshMaterialShader* Shader,
	const EVertexInputStreamType InputStreamType,
	ERHIFeatureLevel::Type FeatureLevel,
	const FVertexFactory* VertexFactory,
	const FMeshBatchElement& BatchElement,
	FMeshDrawSingleShaderBindings& ShaderBindings,
	FVertexInputStreamArray& VertexStreams) const
{
	const FVoxelVertexFactory& VoxelVertexFactory = static_cast<const FVoxelVertexFactory&>(*VertexFactory);
	ensure(VoxelVertexFactory.GlobalParametersUniformBuffer);
    ShaderBindings.Add(Shader->GetUniformBufferParameter<FVoxelVertexFactoryGlobalParameters>(), VoxelVertexFactory.GlobalParametersUniformBuffer);
}

IMPLEMENT_TYPE_LAYOUT(FVoxelVertexFactoryShaderParameters);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelVertexFactory::InitRHI(FRHICommandListBase& RHICmdList)
{
	VOXEL_FUNCTION_COUNTER();

	FVertexFactory::InitRHI(RHICmdList);

	const auto AddDeclaration = [&](const EVertexInputStreamType Type)
	{
		FVertexDeclarationElementList Elements;
		AddPrimitiveIdStreamElement(Type, Elements, 0, 0);
		InitDeclaration(Elements, Type);
	};

	AddDeclaration(EVertexInputStreamType::Default);
	AddDeclaration(EVertexInputStreamType::PositionOnly);
	AddDeclaration(EVertexInputStreamType::PositionAndNormalOnly);

	if (!ensure(RenderData))
	{
		return;
	}

	const FVoxelMegaMaterialProxy& MegaMaterialProxy = *RenderData->MegaMaterialProxy;

	AttributesIndices_Texture = MegaMaterialProxy.GetAttributeIndicesBufferPool().GetTextureRHI_RenderThread();
	Materials_Texture = MegaMaterialProxy.GetMaterialBufferPool().GetTextureRHI_RenderThread();

	if (!ensureVoxelSlow(Materials_Texture))
	{
		Materials_Texture = GBlackTexture->TextureRHI;
	}
	if (!ensureVoxelSlow(AttributesIndices_Texture))
	{
		AttributesIndices_Texture = GBlackTexture->TextureRHI;
	}

	FVoxelVertexFactoryGlobalParameters Parameters;
	Parameters.VoxelDebugMode = VoxelDebugMode;

	Parameters.Vertices = Vertices;
	Parameters.Normals = Normals;
	Parameters.Indices = Indices;

	Parameters.NumAttributes = 1 + MegaMaterialProxy.NumMetadatas();
	Parameters.AttributesIndicesOffset = RenderData->AttributesIndicesOffset;

	Parameters.AttributesIndices_Texture = AttributesIndices_Texture;
	Parameters.AttributesIndices_TextureSizeLog2 = MegaMaterialProxy.GetAttributeIndicesBufferPool().GetTextureSizeLog2_RenderThread();

	Parameters.Materials_Texture = Materials_Texture;
	Parameters.Materials_TextureSizeLog2 = MegaMaterialProxy.GetMaterialBufferPool().GetTextureSizeLog2_RenderThread();

	GlobalParametersUniformBuffer = TUniformBufferRef<FVoxelVertexFactoryGlobalParameters>::CreateUniformBufferImmediate(Parameters, UniformBuffer_MultiFrame);
}

void FVoxelVertexFactory::ReleaseRHI()
{
	VOXEL_FUNCTION_COUNTER();

	FVertexFactory::ReleaseRHI();

	GlobalParametersUniformBuffer.SafeRelease();
}

void FVoxelVertexFactory::ModifyCompilationEnvironment(
	const FVertexFactoryShaderPermutationParameters& Parameters,
	FShaderCompilerEnvironment& OutEnvironment)
{
	OutEnvironment.SetDefine(TEXT("VF_SUPPORTS_PRIMITIVE_SCENE_DATA"),
		UseGPUScene(
			Parameters.Platform,
			GetMaxSupportedFeatureLevel(Parameters.Platform)));
}

void FVoxelVertexFactory::GetPSOPrecacheVertexFetchElements(
	const EVertexInputStreamType VertexInputStreamType,
	FVertexDeclarationElementList& Elements)
{
	Elements.Add(FVertexElement(0, 0, VET_UInt, 0, 0, true));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<EVoxelVertexFactoryType Type>
bool TVoxelVertexFactory<Type>::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
{
	if (!FVoxelMaterialUsage::ShouldCompilePermutation(Parameters.MaterialParameters))
	{
		return false;
	}

	if (!FVoxelUtilities::ShouldCompileBarycentricsSemantic(Parameters.Platform))
	{
		if (Type == EVoxelVertexFactoryType::WithBarycentrics_BasePass ||
			Type == EVoxelVertexFactoryType::WithBarycentrics_BasePass_Debug)
		{
			return false;
		}
	}

	return Parameters.MaterialParameters.MaterialDomain == MD_Surface;
}

template<EVoxelVertexFactoryType Type>
void TVoxelVertexFactory<Type>::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FVoxelVertexFactory::ModifyCompilationEnvironment(Parameters, OutEnvironment);

	OutEnvironment.SetDefine(TEXT("VOXEL_VERTEX_FACTORY"), true);

	switch (Type)
	{
	default: check(false);
	case EVoxelVertexFactoryType::NoBarycentrics_BasePass:
	{
		OutEnvironment.SetDefine(TEXT("WITH_BARYCENTRICS"), false);
		OutEnvironment.SetDefine(TEXT("VOXEL_MESH_VERTEX_FACTORY"), true);
	}
	break;
	case EVoxelVertexFactoryType::WithBarycentrics_BasePass:
	{
		OutEnvironment.SetDefine(TEXT("WITH_BARYCENTRICS"), true);
		OutEnvironment.SetDefine(TEXT("VOXEL_MESH_VERTEX_FACTORY"), true);
	}
	break;
	case EVoxelVertexFactoryType::WithBarycentrics_BasePass_Debug:
	{
		OutEnvironment.SetDefine(TEXT("WITH_BARYCENTRICS"), true);
		OutEnvironment.SetDefine(TEXT("VOXEL_MESH_VERTEX_FACTORY"), true);
		OutEnvironment.SetDefine(TEXT("VOXEL_MEGA_MATERIAL_DEBUG"), true);
	}
	break;
	}
}

#define DEFINE(Type) \
	using TVoxelVertexFactory_ ## Type = TVoxelVertexFactory<EVoxelVertexFactoryType::Type>; \
	\
	IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(TVoxelVertexFactory_ ## Type, SF_Pixel, FVoxelVertexFactoryShaderParameters); \
	IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(TVoxelVertexFactory_ ## Type, SF_Vertex, FVoxelVertexFactoryShaderParameters); \
	RHI_RAYTRACING_ONLY(IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(TVoxelVertexFactory_ ## Type, SF_RayHitGroup, FVoxelVertexFactoryShaderParameters)); \
	\
	IMPLEMENT_TEMPLATE_VERTEX_FACTORY_TYPE(template<>, TVoxelVertexFactory_ ## Type, "/Plugin/Voxel/VoxelVertexFactory.ush", \
		EVertexFactoryFlags::UsedWithMaterials | \
		EVertexFactoryFlags::SupportsDynamicLighting | \
		EVertexFactoryFlags::SupportsPositionOnly | \
		EVertexFactoryFlags::SupportsCachingMeshDrawCommands | \
		EVertexFactoryFlags::SupportsPrimitiveIdStream | \
		EVertexFactoryFlags::SupportsRayTracing | \
		/* TODO Crashes on 1080 EVertexFactoryFlags::SupportsPSOPrecaching */ \
		EVertexFactoryFlags::SupportsLumenMeshCards);

DEFINE(NoBarycentrics_BasePass);
DEFINE(WithBarycentrics_BasePass);
DEFINE(WithBarycentrics_BasePass_Debug);

#undef DEFINE