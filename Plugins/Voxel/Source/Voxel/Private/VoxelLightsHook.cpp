// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.h"
#include "VoxelShaderHook.h"

#if WITH_EDITOR
DECLARE_VOXEL_SHADER_HOOK(VOXEL_API, FVoxelLightsHook);

DEFINE_VOXEL_SHADER_HOOK(
	FVoxelLightsHook,
	"Lights HLSL packaging fix",
	"This hook fixes HLSL lights packaging errors.");

ADD_VOXEL_SHADER_HOOK(
	FVoxelLightsHook,
	"FCC40581D043476DB472B1E42C16A05F",
	"/Engine/Private/CapsuleLightIntegrate.ush",
	R"(
	float3 ToLight = 0.5 * ( Capsule.LightPos[0] + Capsule.LightPos[1] );
	float3 CapsuleAxis = normalize( Capsule.LightPos[1] - Capsule.LightPos[0] );

	float DistanceSqr = dot( ToLight, ToLight );
	float3 ConeAxis = ToLight * rsqrt( DistanceSqr );
	float SineConeSqr = saturate(Pow2(Capsule.Radius) / DistanceSqr);

	FCapsuleSphericalBounds CapsuleBounds = CapsuleGetSphericalBounds(ToLight, CapsuleAxis, Capsule.Radius, Capsule.Length);
	
	const uint NumSets = 3;)",
	R"(
	{
		0,	// Cosine hemisphere
		16,	// GGX
		16,	// Light area
	};
	
	uint2 SobolBase = SobolPixel( SVPos );
	uint2 SobolFrame = SobolIndex( SobolBase, View.StateFrameIndexMod8, 3 );
	
	UNROLL)",
	"const uint NumSamples[ NumSets ] =",
	"\tconst uint NumSamples[ 3 ] =");

ADD_VOXEL_SHADER_HOOK(
	FVoxelLightsHook,
	"C932EEB7D4374755A970AABA7CEF6EFC",
	"/Engine/Private/RectLightIntegrate.ush",
	R"(
	// No-visible rect light due to barn door occlusion
	if (!IsRectVisible(Rect))
		return Lighting;

	FSphericalRect SphericalRect = BuildSphericalRect( Rect );
	
	const uint NumSets = 4;)",
	R"(
	{
		0,	// Cosine hemisphere
		16,	// GGX
		0,	// Light area
		16,	// Spherical rect
	};

	uint2 SobolBase = SobolPixel( SVPos );
	uint2 SobolFrame = SobolIndex( SobolBase, View.StateFrameIndexMod8, 3 );)",
	"const uint NumSamples[ NumSets ] =",
	"\tconst uint NumSamples[ 4 ] =");
#endif