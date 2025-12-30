// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Height/VoxelHeightSurfaceTypeChunk.h"
#include "Surface/VoxelSurfaceTypeInterface.h"

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelHeightSurfaceTypeChunk);
DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelHeightSurfaceType_Memory);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelHeightSurfaceTypeChunk::FVoxelHeightSurfaceTypeChunk()
{
	UpdateStats();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelHeightSurfaceTypeChunk::GetSurfaceType(
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

void FVoxelHeightSurfaceTypeChunk::Serialize(FArchive& Ar)
{
	VOXEL_FUNCTION_COUNTER();

	using FVersion = DECLARE_VOXEL_VERSION
	(
		FirstVersion
	);

	int32 Version = FVersion::LatestVersion;
	Ar << Version;

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

int64 FVoxelHeightSurfaceTypeChunk::GetAllocatedSize() const
{
	int64 AllocatedSize = 0;
	AllocatedSize += Alphas.GetAllocatedSize();
	AllocatedSize += Layers.GetAllocatedSize();
	AllocatedSize += UsedSurfaceTypes.GetAllocatedSize();
	return AllocatedSize;
}

TVoxelRefCountPtr<FVoxelHeightSurfaceTypeChunk> FVoxelHeightSurfaceTypeChunk::Clone() const
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelHeightSurfaceTypeChunk* Result = new FVoxelHeightSurfaceTypeChunk();
	Result->Alphas = Alphas;
	Result->Layers = Layers;
	Result->UsedSurfaceTypes = UsedSurfaceTypes;

	Result->UpdateStats();
	return Result;
}