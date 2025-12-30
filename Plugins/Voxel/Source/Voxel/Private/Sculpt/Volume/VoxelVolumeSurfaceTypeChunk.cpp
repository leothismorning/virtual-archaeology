// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Volume/VoxelVolumeSurfaceTypeChunk.h"
#include "Sculpt/Volume/VoxelVolumeSculptVersion.h"
#include "Surface/VoxelSurfaceTypeInterface.h"

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelVolumeSurfaceTypeChunk);
DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelVolumeSurfaceType_Memory);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelVolumeSurfaceTypeChunk::FVoxelVolumeSurfaceTypeChunk()
{
	UpdateStats();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelVolumeSurfaceTypeChunk::GetSurfaceType(
	const int32 Index,
	float& OutAlpha,
	FVoxelSurfaceTypeBlend& OutSurfaceType) const
{
	int32 NumLayers = 0;
	TVoxelStaticArray<FVoxelSurfaceTypeBlendLayer, FVoxelSurfaceTypeBlend::MaxLayers> UnpackedLayers{ NoInit };

	for (const FLayer& Layer : Layers)
	{
		const uint8 Weight = Layer.Weights[Index];
		if (Weight == 0)
		{
			continue;
		}

		UnpackedLayers[NumLayers++] = FVoxelSurfaceTypeBlendLayer
		{
			UsedSurfaceTypes[Layer.Types[Index]],
			FVoxelUtilities::UINT8ToFloat(Weight)
		};
	}

	OutAlpha = FVoxelUtilities::UINT8ToFloat(Alphas[Index]);
	OutSurfaceType.InitializeFromLayers(UnpackedLayers.View().LeftOf(NumLayers));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelVolumeSurfaceTypeChunk::Serialize(
	FArchive& Ar,
	const int32 Version)
{
	VOXEL_FUNCTION_COUNTER();

	if (Version < FVoxelVolumeSculptVersion::MergeVersions)
	{
		using FVersion = DECLARE_VOXEL_VERSION
		(
			FirstVersion
		);

		int32 LegacyVersion = FVersion::LatestVersion;
		Ar << LegacyVersion;
	}

	Ar << Alphas;
	Ar << Layers;

	TVoxelArray<UVoxelSurfaceTypeInterface*> SurfaceTypeObjects;

	if (Ar.IsSaving())
	{
		SurfaceTypeObjects.Reserve(UsedSurfaceTypes.Num());

		for (const FVoxelSurfaceType& SurfaceType : UsedSurfaceTypes)
		{
			UVoxelSurfaceTypeInterface* SurfaceTypeObject = SurfaceType.GetSurfaceTypeInterface().Resolve();
			ensureVoxelSlow(SurfaceTypeObject);
			SurfaceTypeObjects.Add_EnsureNoGrow(SurfaceTypeObject);
		}
	}

	Ar << SurfaceTypeObjects;

	if (Ar.IsLoading())
	{
		UsedSurfaceTypes.Reset(SurfaceTypeObjects.Num());

		for (UVoxelSurfaceTypeInterface* SurfaceTypeObject : SurfaceTypeObjects)
		{
			UsedSurfaceTypes.Add_EnsureNoGrow(FVoxelSurfaceType(SurfaceTypeObject));
		}
	}
}

int64 FVoxelVolumeSurfaceTypeChunk::GetAllocatedSize() const
{
	int64 AllocatedSize = 0;
	AllocatedSize += Alphas.GetAllocatedSize();
	AllocatedSize += Layers.GetAllocatedSize();
	AllocatedSize += UsedSurfaceTypes.GetAllocatedSize();
	return AllocatedSize;
}

TVoxelRefCountPtr<FVoxelVolumeSurfaceTypeChunk> FVoxelVolumeSurfaceTypeChunk::Clone() const
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelVolumeSurfaceTypeChunk* Result = new FVoxelVolumeSurfaceTypeChunk();
	Result->Alphas = Alphas;
	Result->Layers = Layers;
	Result->UsedSurfaceTypes = UsedSurfaceTypes;

	Result->UpdateStats();
	return Result;
}