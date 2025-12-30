// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelLayersBlueprintLibrary.h"
#include "VoxelQuery.h"
#include "VoxelLayers.h"
#include "Buffer/VoxelBaseBuffers.h"
#include "Surface/VoxelSurfaceTypeTable.h"
#include "Surface/VoxelSurfaceTypeBlendBuffer.h"

bool UVoxelLayersBlueprintLibrary::QueryVoxelLayer(
	UObject* WorldContextObject,
	const FVoxelStackLayer Layer,
	const FVector Position,
	const bool bQuerySurfaceTypes,
	const TArray<UVoxelFloatMetadata*> MetadatasToQuery,
	const int32 LOD,
	FVoxelQueryResult& Result)
{
	VOXEL_FUNCTION_COUNTER();
	ensure(IsInGameThread());

	TArray<FVoxelQueryResult> Values;
	if (!MultiQueryVoxelLayer(
		WorldContextObject,
		Layer,
		{ Position },
		bQuerySurfaceTypes,
		MetadatasToQuery,
		LOD,
		Values))
	{
		return false;
	}

	Result = Values[0];

	return true;
}

bool UVoxelLayersBlueprintLibrary::MultiQueryVoxelLayer(
	UObject* WorldContextObject,
	const FVoxelStackLayer Layer,
	const TArray<FVector> Positions,
	const bool bQuerySurfaceTypes,
	const TArray<UVoxelFloatMetadata*> MetadatasToQuery,
	const int32 LOD,
	TArray<FVoxelQueryResult>& Result)
{
	VOXEL_FUNCTION_COUNTER_NUM(Positions.Num());
	ensure(IsInGameThread());

	if (!WorldContextObject)
	{
		VOXEL_MESSAGE(Error, "WorldContextObject is null");
		return false;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		VOXEL_MESSAGE(Error, "World is null");
		return false;
	}

	const TSharedRef<FVoxelLayers> Layers = FVoxelLayers::Get(World);
	const TSharedRef<FVoxelSurfaceTypeTable> SurfaceTypeTable = FVoxelSurfaceTypeTable::Get();

	Result = MultiQueryVoxelLayer_AnyThread(
		*Layers,
		*SurfaceTypeTable,
		FVoxelDependencyCollector::Null,
		Positions,
		FVoxelWeakStackLayer(Layer),
		bQuerySurfaceTypes,
		FVoxelFloatMetadataRef::GetUniqueValidRefs(MetadatasToQuery),
		LOD);

	return true;
}

TArray<FVoxelQueryResult> UVoxelLayersBlueprintLibrary::MultiQueryVoxelLayer_AnyThread(
	const FVoxelLayers& Layers,
	const FVoxelSurfaceTypeTable& SurfaceTypeTable,
	FVoxelDependencyCollector& DependencyCollector,
	const TArray<FVector>& Positions,
	const FVoxelWeakStackLayer& WeakLayer,
	const bool bQuerySurfaceTypes,
	const TConstVoxelArrayView<FVoxelFloatMetadataRef> MetadatasToQuery,
	const int32 LOD)
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelDoubleVectorBuffer PositionsBuffer;
	{
		PositionsBuffer.Allocate(Positions.Num());

		for (int32 Index = 0; Index < Positions.Num(); Index++)
		{
			PositionsBuffer.Set(Index, Positions[Index]);
		}
	}

	FVoxelSurfaceTypeBlendBuffer SurfaceTypes;
	if (bQuerySurfaceTypes)
	{
		SurfaceTypes.AllocateZeroed(Positions.Num());
	}

	TVoxelMap<FVoxelMetadataRef, TSharedRef<FVoxelFloatBuffer>> MetadataToBuffer;
	MetadataToBuffer.Reserve(MetadatasToQuery.Num());

	for (const FVoxelFloatMetadataRef& MetadataToQuery : MetadatasToQuery)
	{
		const TSharedRef<FVoxelFloatBuffer> Buffer = MakeShared<FVoxelFloatBuffer>();
		Buffer->AllocateZeroed(Positions.Num());
		MetadataToBuffer.Add_EnsureNew(MetadataToQuery, Buffer);
	}

	const FVoxelQuery Query(
		LOD,
		Layers,
		SurfaceTypeTable,
		DependencyCollector);

	FVoxelFloatBuffer Values;
	if (WeakLayer.Type == EVoxelLayerType::Height)
	{
		FVoxelDoubleVector2DBuffer Positions2D;
		Positions2D.X = PositionsBuffer.X;
		Positions2D.Y = PositionsBuffer.Y;

		Values = Query.SampleHeightLayer(
			WeakLayer,
			Positions2D,
			SurfaceTypes.View(),
			MetadataToBuffer);
	}
	else
	{
		Values = Query.SampleVolumeLayer(
			WeakLayer,
			PositionsBuffer,
			SurfaceTypes.View(),
			MetadataToBuffer);
	}

	TArray<FVoxelQueryResult> Result;
	FVoxelUtilities::SetNum(Result, Positions.Num());

	for (int32 Index = 0; Index < Positions.Num(); Index++)
	{
		FVoxelQueryResult& QueryResult = Result[Index];
		QueryResult.Value = Values[Index];

		if (FVoxelUtilities::IsNaN(Values[Index]))
		{
			continue;
		}

		if (bQuerySurfaceTypes)
		{
			const FVoxelSurfaceTypeBlend SurfaceType = SurfaceTypes[Index];
			if (SurfaceType.IsNull())
			{
				QueryResult.SurfaceType = nullptr;
			}
			else
			{
				QueryResult.SurfaceType = SurfaceType.GetTopLayer().Type.GetSurfaceTypeInterface().Resolve_Ensured();
			}
		}

		for (const FVoxelFloatMetadataRef& MetadataToQuery : MetadatasToQuery)
		{
			QueryResult.Metadata.Add(
				MetadataToQuery.GetMetadata().Resolve(),
				(*MetadataToBuffer[MetadataToQuery])[Index]);
		}
	}

	return Result;
}