// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMetadataRef.h"
#include "Surface/VoxelSurfaceTypeBlend.h"
#include "VoxelVolumeModifier.generated.h"

class UVoxelMetadata;

USTRUCT()
struct VOXEL_API FVoxelVolumeModifier : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	virtual void Initialize_GameThread();
	virtual FVoxelBox GetBounds() const VOXEL_PURE_VIRTUAL({});

	virtual void GetUsage(
		bool& bWritesDistances,
		bool& bWritesSurfaceTypes,
		TVoxelSet<FVoxelMetadataRef>& MetadataRefsToWrite) const
	{
		bWritesDistances = true;
	}

	struct FSurfaceTypes
	{
		TVoxelArray<float> Alphas;
		TVoxelArray<FVoxelSurfaceTypeBlend> SurfaceTypes;
	};
	struct FMetadata
	{
		TVoxelArray<float> Alphas;
		TSharedPtr<FVoxelBuffer> Buffer;
	};

	struct FData
	{
		TVoxelArrayView<float> Distances;
		FIntVector Size = FIntVector(ForceInit);
		TSharedPtr<FSurfaceTypes> SurfaceTypes;
		TVoxelMap<FVoxelMetadataRef, TSharedPtr<FMetadata>> MetadataRefToMetadata;
		FVoxelIntBox Indices;
		FMatrix IndexToWorld = FMatrix(ForceInit);

		template<typename LambdaType>
		requires LambdaHasSignature_V<LambdaType, float(float, const FVector&)>
		void Apply(LambdaType Lambda) const
		{
			VOXEL_FUNCTION_COUNTER_NUM(Indices.Count_int32());

			Voxel::ParallelFor(Indices.GetZ(), [&](const int32 IndexZ)
			{
				for (int32 IndexY = Indices.Min.Y; IndexY < Indices.Max.Y; IndexY++)
				{
					for (int32 IndexX = Indices.Min.X; IndexX < Indices.Max.X; IndexX++)
					{
						float& Distance = Distances[IndexX + Size.X * IndexY + Size.X * Size.Y * IndexZ];
						const FVector Position = IndexToWorld.TransformPosition(FVector(IndexX, IndexY, IndexZ));

						Distance = Lambda(MakeCopy(Distance), Position);
					}
				}
			});
		}
	};
	virtual void Apply(const FData& Data) const VOXEL_PURE_VIRTUAL();

private:
	bool bInitialized = false;
};