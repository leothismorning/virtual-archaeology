// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMetadataRef.h"
#include "Surface/VoxelSurfaceTypeBlend.h"
#include "VoxelHeightModifier.generated.h"

class UVoxelMetadata;

USTRUCT()
struct VOXEL_API FVoxelHeightModifier : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	virtual void Initialize_GameThread();
	virtual FVoxelBox2D GetBounds() const VOXEL_PURE_VIRTUAL({});

	virtual void GetUsage(
		bool& bWritesHeights,
		bool& bWritesSurfaceTypes,
		TVoxelSet<FVoxelMetadataRef>& MetadataRefsToWrite) const
	{
		bWritesHeights = true;
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
		TVoxelArrayView<float> Heights;
		FIntPoint Size = FIntPoint(ForceInit);
		TSharedPtr<FSurfaceTypes> SurfaceTypes;
		TVoxelMap<FVoxelMetadataRef, TSharedPtr<FMetadata>> MetadataRefToMetadata;
		FVoxelIntBox2D Indices;
		FTransform2d IndexToWorld;

		template<typename LambdaType>
		requires LambdaHasSignature_V<LambdaType, float(float, const FVector2D&)>
		void Apply(LambdaType Lambda) const
		{
			VOXEL_FUNCTION_COUNTER_NUM(Indices.Count_int32());

			Voxel::ParallelFor(Indices.GetY(), [&](const int32 IndexY)
			{
				for (int32 IndexX = Indices.Min.X; IndexX < Indices.Max.X; IndexX++)
				{
					float& Height = Heights[IndexX + Size.X * IndexY];
					const FVector2D Position = IndexToWorld.TransformPoint(FVector2D(IndexX, IndexY));

					Height = Lambda(MakeCopy(Height), Position);
				}
			});
		}
	};
	virtual void Apply(const FData& Data) const VOXEL_PURE_VIRTUAL();

private:
	bool bInitialized = false;
};