// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMaterialHook.h"

#if WITH_EDITOR
DEFINE_VOXEL_SHADER_HOOK(
	FVoxelMaterialHook,
	"Voxel Material",
	"This is required to render voxel materials");

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ADD_VOXEL_SHADER_HOOK(
	FVoxelMaterialHook,
	"12505939C4CB4E7BB2830220C1DD0E92",
	"/Engine/Private/MaterialTemplate.ush",
	R"(
##if CLOUD_LAYER_PIXEL_SHADER
	float CloudSampleAltitude;
	float CloudSampleAltitudeInLayer;
	float CloudSampleNormAltitudeInLayer;
	float4 VolumeSampleConservativeDensity;
	float ShadowSampleDistance;

	float3 CloudEmptySpaceSkippingSphereCenterWorldPosition;
	float  CloudEmptySpaceSkippingSphereRadius;
##endif
	)",
	R"(
##if TEMPLATE_USES_SUBSTRATE
	FSharedLocalBases SharedLocalBases;
	FSubstrateTree SubstrateTree;

##if SUBSTRATE_USE_FULLYSIMPLIFIED_MATERIAL == 1
	FSharedLocalBases SharedLocalBasesFullySimplified;
	FSubstrateTree SubstrateTreeFullySimplified;
	)",
	"",
	R"(
	uint Voxel_DebugMode;
	uint Voxel_PageIndex;
	uint Voxel_ClusterIndex;
	uint3 Voxel_TriIndices;
	float3 Voxel_Barycentrics;
	uint Voxel_LayerIndex;
	float Voxel_Displacement;

	uint4 Voxel_LayerMask;
	float Voxel_LayerWeights[8];
	uint3 Voxel_AttributesIndicesIndices;
	float3 Voxel_FlatNormal;

##define MATERIAL_PIXEL_PARAMETERS_VOXEL_VERSION 7
	)");

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ADD_VOXEL_SHADER_HOOK(
	FVoxelMaterialHook,
	"EE0CDA91957940039914A3652A66E645",
	"/Engine/Private/MaterialTemplate.ush",
	R"(
	FMaterialAttributes MaterialAttributes;

	/** Cached primitive and instance data */
	FSceneDataIntermediates SceneData;
	)",
	R"(
	// FIXME: just for compatibility with assets that use custom HLSL expressions, will be removed once we fix up all these assets
	// Index into View.PrimitiveSceneData
	uint PrimitiveId;

	bool bEvaluateWorldPositionOffset;
	)",
	"",
	R"(
	uint Voxel_DebugMode;
	uint Voxel_PageIndex;
	uint Voxel_ClusterIndex;
	uint3 Voxel_VertexIndex;
	uint Voxel_LayerIndex;
	float Voxel_Displacement;
	uint4 Voxel_LayerMask;
	float Voxel_LayerWeights[8];
	uint3 Voxel_AttributesIndicesIndex;

##define MATERIAL_VERTEX_PARAMETERS_VOXEL_VERSION 5
	)");

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ADD_VOXEL_SHADER_HOOK(
	FVoxelMaterialHook,
	"6F912F1438B246498E18FDD90C717824",
	"/Engine/Private/Nanite/NaniteRasterizationCommon.ush",
	R"(
		VertexParameters = MakeInitializedMaterialVertexParameters();
		SetVertexParameterInstanceData(VertexParameters, InstanceData, PrimitiveData, true /* WPO */);
		SetVertexParameterAttributeData(VertexParameters, InputVert, InstanceDynamicData.LocalToTranslatedWorld, LocalToWorld);
	)",
	R"(
	##endif
	}
	)",
	"",
	R"(
##if MATERIAL_PIXEL_PARAMETERS_VOXEL_VERSION == 7
		VertexParameters.Voxel_PageIndex = VisibleCluster.PageIndex;
		VertexParameters.Voxel_ClusterIndex = VisibleCluster.ClusterIndex;
		VertexParameters.Voxel_VertexIndex = InputVert.VertIndex;
##endif
	)");

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ADD_VOXEL_SHADER_HOOK(
	FVoxelMaterialHook,
	"7B21C96209BB49F3B03A0EB09D0711EE",
	"/Engine/Private/Nanite/NaniteRasterizationCommon.ush",
	R"(
	// Hopefully never used and will be dead code eliminated. TODO VT feedback needs this right now :(
	float4 SvPosition;
	SvPosition.xyz = BaseClip.xyz / BaseClip.w;
	SvPosition.xy = ( float2(0.5, -0.5) * SvPosition.xy + 0.5 ) * NaniteView.ViewSizeAndInvSize.xy + NaniteView.ViewRect.xy;
	SvPosition.w = 1;

	FVertexFactoryInterpolantsVSToPS Interpolants = (FVertexFactoryInterpolantsVSToPS)0;
	FMaterialPixelParameters MaterialParameters = FetchNaniteMaterialPixelParameters( PrimitiveData, InstanceData, InstanceDynamicData, NaniteView, TransformedTri, Cluster, Barycentrics, Interpolants, SvPosition );
	)",
	R"(
		// Now we want to override UV derivatives to be based on uniform UV density so that there are no cracks
	##if NUM_TEX_COORD_INTERPOLATORS > 0
		for (uint TexCoordIndex = 0; TexCoordIndex < NUM_TEX_COORD_INTERPOLATORS; ++TexCoordIndex)
		{
			float2 Deriv = CalculateUVDerivativeForDomainPoint(
				NaniteView,
				BaseClip.w,
				InstanceData.NonUniformScale.w,
				UVDensities[TexCoordIndex] );
			MaterialParameters.TexCoords_DDX[TexCoordIndex] = Deriv;
			MaterialParameters.TexCoords_DDY[TexCoordIndex] = Deriv;
		}
	##endif
	)",
	"",
	R"(
##if MATERIAL_PIXEL_PARAMETERS_VOXEL_VERSION == 7
		MaterialParameters.Voxel_PageIndex = VisibleCluster.PageIndex;
		MaterialParameters.Voxel_ClusterIndex = VisibleCluster.ClusterIndex;
		MaterialParameters.Voxel_TriIndices = uint3(
			TransformedTri.Verts[0].VertIndex,
			TransformedTri.Verts[1].VertIndex,
			TransformedTri.Verts[2].VertIndex);
		MaterialParameters.Voxel_Barycentrics = Barycentrics.Value;
##endif
	)");

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ADD_VOXEL_SHADER_HOOK(
	FVoxelMaterialHook,
	"B7DCE9F0880548BFA6866080484498E9",
	"/Engine/Private/Nanite/NaniteVertexFactory.ush",
	UE_506_SWITCH(
	R"(
				Tri.Verts[0].TangentBasis.TangentZ,
				Tri.Verts[1].TangentBasis.TangentZ,
				Tri.Verts[2].TangentBasis.TangentZ );
		##else
			Barycentrics = CalculateTriangleBarycentrics(PixelClip, Tri.Verts[0].PointClip, Tri.Verts[1].PointClip, Tri.Verts[2].PointClip, NaniteView.ViewSizeAndInvSize.zw);
		##endif
	}

	Result = FetchNaniteMaterialPixelParameters(PrimitiveData, InstanceData, InstanceDynamicData, NaniteView, Tri, Cluster, Barycentrics, Interpolants, SvPosition);
	)",
	R"(
	Result = FetchNaniteMaterialPixelParameters(PrimitiveData, InstanceData, InstanceDynamicData, NaniteView, Tri, Cluster, Barycentrics, Interpolants, SvPosition);

##if NUM_TEX_COORD_INTERPOLATORS > 0
	if( Cluster.bVoxel )
	{
		float Depth = NaniteView.ViewToClip[3][2] / ( SvPosition.z - NaniteView.ViewToClip[2][2] );
		float4 UVDensities = GetMaterialUVDensities(Cluster, InstanceData.PrimitiveId, TriIndex);

		UNROLL
		for (uint TexCoordIndex = 0; TexCoordIndex < NUM_TEX_COORD_INTERPOLATORS; TexCoordIndex++)
		{
			// TODO Don't use NaniteView.LODScale
			float dUV_dXY = rcp( GetProjectedEdgeLengthAtDepth( UVDensities[TexCoordIndex] * InstanceData.NonUniformScale.w, Depth, NaniteView ) );
			Result.TexCoords_DDX[TexCoordIndex] = dUV_dXY;
			Result.TexCoords_DDY[TexCoordIndex] = dUV_dXY;
		}
	}
##endif
	)"),
	R"(
	}

	return Result;
}

##if IS_NANITE_SHADING_PASS
	)",
	"",
	R"(
##if MATERIAL_PIXEL_PARAMETERS_VOXEL_VERSION == 7
	Result.Voxel_PageIndex = VisibleCluster.PageIndex;
	Result.Voxel_ClusterIndex = VisibleCluster.ClusterIndex;
	Result.Voxel_TriIndices = uint3(
		Tri.Verts[0].VertIndex,
		Tri.Verts[1].VertIndex,
		Tri.Verts[2].VertIndex);
	Result.Voxel_Barycentrics = Barycentrics.Value;
	Result.Voxel_FlatNormal = normalize(cross(
		Tri.Verts[2].PointPostDeform - Tri.Verts[0].PointPostDeform,
		Tri.Verts[1].PointPostDeform - Tri.Verts[0].PointPostDeform));
##endif
	)");

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ADD_VOXEL_SHADER_HOOK(
	FVoxelMaterialHook,
	"729AA07C94854257BC306DF7D8C6F886",
	"/Engine/Private/Nanite/NaniteAttributeDecode.ush",
	R"(
		else
		{
			if (TexCoordIndex == 0)
			{
				AttributeData.TangentToWorld = float3x3(float3(0, 0, 0), float3(0, 0, 0), DFMultiplyVector(TangentZ * InstanceData.InvNonUniformScale.z, InstanceData.LocalToWorld));
	)",
	R"(
			}
		}

		AttributeData.TexCoords[TexCoordIndex] = TexCoord;
	}

	return AttributeData;
	)",
	"",
	R"(
##ifdef VOXEL_NANITE_MATERIAL
##if VOXEL_NANITE_MATERIAL
				const float Sign = TangentZ.z >= 0 ? 1 : -1;
				const float a = -rcp(Sign + TangentZ.z);
				const float b = TangentZ.x * TangentZ.y * a;

				const float3 TangentX = float3(1 + Sign * a * Pow2(TangentZ.x), Sign * b, -Sign * TangentZ.x);
				const float3 TangentY = float3(b, Sign + a * Pow2(TangentZ.y), -TangentZ.y);

				AttributeData.UnMirrored = 1;

				// Should be Pow2(InvScale) but that requires renormalization
				float3x3 LocalToWorldNoScale = DFToFloat3x3(InstanceData.LocalToWorld);
				float3 InvScale = InstanceData.InvNonUniformScale;
				LocalToWorldNoScale[0] *= InvScale.x;
				LocalToWorldNoScale[1] *= InvScale.y;
				LocalToWorldNoScale[2] *= InvScale.z;
				AttributeData.TangentToWorld = mul(float3x3(TangentX, TangentY, TangentZ), LocalToWorldNoScale);
##endif
##endif
	)");

#endif