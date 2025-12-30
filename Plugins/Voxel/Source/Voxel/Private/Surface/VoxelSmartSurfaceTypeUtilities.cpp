// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Surface/VoxelSmartSurfaceTypeUtilities.h"
#include "Surface/VoxelSmartSurfaceType.h"
#include "Surface/VoxelSurfaceTypeTable.h"
#include "Surface/VoxelSurfaceTypeBlend.h"
#include "Surface/VoxelSmartSurfaceProxy.h"
#include "Surface/VoxelSurfaceTypeBlendBuffer.h"
#include "Surface/VoxelSmartSurfaceFunctionLibrary.h"
#include "Surface/VoxelOutputNode_OutputSurface.h"
#include "VoxelQuery.h"
#include "VoxelGraphQuery.h"
#include "VoxelGraphContext.h"
#include "VoxelGraphPositionParameter.h"
#include "Buffer/VoxelDoubleBuffers.h"
#include "Graphs/VoxelStampGraphParameters.h"

void FVoxelSmartSurfaceTypeUtilities::Resolve(
	const int32 LOD,
	const FVoxelWeakStackLayer& WeakLayer,
	FVoxelDependencyCollector& DependencyCollector,
	const FVoxelLayers& Layers,
	const FVoxelSurfaceTypeTable& SurfaceTypeTable,
	const FVoxelDoubleVectorBuffer& VertexPositions,
	const FVoxelVectorBuffer& VertexNormals,
	const TVoxelArrayView<FVoxelSurfaceTypeBlend> SurfaceTypeBlends)
{
	VOXEL_SCOPE_COUNTER_NUM("Smart Surfaces", SurfaceTypeBlends.Num());
	check(VertexPositions.Num() == SurfaceTypeBlends.Num() || VertexPositions.IsConstant());
	check(VertexNormals.Num() == SurfaceTypeBlends.Num() || VertexNormals.IsConstant());

#if VOXEL_DEBUG
	ON_SCOPE_EXIT
	{
		for (const FVoxelSurfaceTypeBlend& SurfaceTypeBlend : SurfaceTypeBlends)
		{
			for (const FVoxelSurfaceTypeBlendLayer& Layer : SurfaceTypeBlend.GetLayers())
			{
				ensure(Layer.Type.GetClass() == FVoxelSurfaceType::EClass::SurfaceTypeAsset);
			}
		}
	};
#endif

	if (SurfaceTypeTable.SurfaceTypeToSmartSurfaceProxy.Num() == 0)
	{
		return;
	}

	TVoxelMap<FVoxelSurfaceType, TVoxelChunkedArray<int32>> SurfaceTypeToVerticesToCompute;

	bool bAllVerticesComplete = true;
	FVoxelBitArray IncompleteVertices;
	{
		VOXEL_SCOPE_COUNTER("First pass");

		IncompleteVertices.SetNum(SurfaceTypeBlends.Num(), false);

		for (int32 VertexIndex = 0; VertexIndex < SurfaceTypeBlends.Num(); VertexIndex++)
		{
			const FVoxelSurfaceTypeBlend& SurfaceTypeBlend = SurfaceTypeBlends[VertexIndex];

			bool bIsComplete = true;
			for (const FVoxelSurfaceTypeBlendLayer& Layer : SurfaceTypeBlend.GetLayers())
			{
				if (Layer.Type.GetClass() == FVoxelSurfaceType::EClass::SurfaceTypeAsset)
				{
					continue;
				}

				bIsComplete = false;
				SurfaceTypeToVerticesToCompute.FindOrAdd(Layer.Type).Add(VertexIndex);
			}

			if (bIsComplete)
			{
				continue;
			}

			bAllVerticesComplete = false;

			checkVoxelSlow(!IncompleteVertices[VertexIndex]);
			IncompleteVertices[VertexIndex] = true;
		}
	}

	if (bAllVerticesComplete)
	{
		return;
	}

	TVoxelArray<TVoxelMap<FVoxelSurfaceType, FVoxelSurfaceTypeBlend>> VertexIndexToSurfaceTypeToSurfaceTypeBlend;
	FVoxelUtilities::SetNum(VertexIndexToSurfaceTypeToSurfaceTypeBlend, SurfaceTypeBlends.Num());

	FVoxelDoubleVectorBuffer Positions;
	FVoxelVectorBuffer Normals;

	int32 PassIndex = 0;

	while (!bAllVerticesComplete)
	{
		VOXEL_SCOPE_COUNTER_FORMAT("Pass %d Num=%d", PassIndex, IncompleteVertices.CountSetBits());
		PassIndex++;

		if (PassIndex > 64)
		{
			VOXEL_MESSAGE(Error, "Failed to resolve smart surfaces: smart surfaces are likely recursive");

			for (FVoxelSurfaceTypeBlend& SurfaceTypeBlend : SurfaceTypeBlends)
			{
				for (const FVoxelSurfaceTypeBlendLayer& Layer : SurfaceTypeBlend.GetLayers())
				{
					if (Layer.Type.GetClass() != FVoxelSurfaceType::EClass::SurfaceTypeAsset)
					{
						SurfaceTypeBlend = {};
						break;
					}
				}
			}

			break;
		}

		for (auto& It : SurfaceTypeToVerticesToCompute)
		{
			const FVoxelSurfaceType SurfaceType = It.Key;
			TVoxelChunkedArray<int32>& VerticesToCompute = It.Value;
			ON_SCOPE_EXIT
			{
				VerticesToCompute.Reset();
			};

			if (VerticesToCompute.Num() == 0)
			{
				continue;
			}

			checkVoxelSlow(SurfaceTypeTable.SurfaceTypeToSmartSurfaceProxy.Contains(SurfaceType));

			const TSharedPtr<FVoxelSmartSurfaceProxy> Proxy = SurfaceTypeTable.SurfaceTypeToSmartSurfaceProxy.FindRef(SurfaceType);
			if (!Proxy)
			{
				// Evaluator failed to compile
				for (int32 Index = 0; Index < VerticesToCompute.Num(); Index++)
				{
					TVoxelMap<FVoxelSurfaceType, FVoxelSurfaceTypeBlend>& SurfaceTypeToSurfaceTypeBlend = VertexIndexToSurfaceTypeToSurfaceTypeBlend[VerticesToCompute[Index]];
					SurfaceTypeToSurfaceTypeBlend.Add_EnsureNew(SurfaceType, FVoxelSurfaceTypeBlend());
				}
				continue;
			}

			DependencyCollector.AddDependency(*Proxy->Dependency);

			VOXEL_SCOPE_COUNTER_FNAME(Proxy->Name);

			{
				VOXEL_SCOPE_COUNTER_NUM("Positions & Normals", VerticesToCompute.Num());

				Positions.Allocate(VerticesToCompute.Num());
				Normals.Allocate(VerticesToCompute.Num());

				for (int32 Index = 0; Index < VerticesToCompute.Num(); Index++)
				{
					const int32 VertexIndex = VerticesToCompute[Index];

					Positions.Set(Index, VertexPositions[VertexIndex]);
					Normals.Set(Index, VertexNormals[VertexIndex]);
				}
			}

			FVoxelGraphContext Context = Proxy->Evaluator.MakeContext(DependencyCollector);

			const FVoxelQuery Query(
				LOD,
				Layers,
				SurfaceTypeTable,
				DependencyCollector);

			FVoxelGraphQueryImpl& GraphQuery = Context.MakeQuery();
			GraphQuery.AddParameter<FVoxelGraphParameters::FLOD>().Value = LOD;
			GraphQuery.AddParameter<FVoxelGraphParameters::FQuery>(Query);
			GraphQuery.AddParameter<FVoxelGraphParameters::FPosition3D>().SetLocalPosition(Positions);
			GraphQuery.AddParameter<FVoxelGraphParameters::FSmartSurface>().Normals = Normals;
			GraphQuery.AddParameter<FVoxelGraphParameters::FSmartSurfaceUniform>().WeakLayer = WeakLayer;

			const TSharedRef<const FVoxelSurfaceTypeBlendBuffer> SurfaceTypeBlend = Proxy->Evaluator->SurfaceTypePin.GetSynchronous(GraphQuery);

			for (int32 Index = 0; Index < VerticesToCompute.Num(); Index++)
			{
				TVoxelMap<FVoxelSurfaceType, FVoxelSurfaceTypeBlend>& SurfaceTypeToSurfaceTypeBlend = VertexIndexToSurfaceTypeToSurfaceTypeBlend[VerticesToCompute[Index]];
				SurfaceTypeToSurfaceTypeBlend.Add_EnsureNew(SurfaceType, (*SurfaceTypeBlend)[Index]);
			}
		}

		bAllVerticesComplete = true;

		IncompleteVertices.ForAllSetBits([&](const int32 VertexIndex)
		{
			FVoxelSurfaceTypeBlend& SurfaceTypeBlend = SurfaceTypeBlends[VertexIndex];

			const TVoxelMap<FVoxelSurfaceType, FVoxelSurfaceTypeBlend>& SurfaceTypeToSurfaceTypeBlend = VertexIndexToSurfaceTypeToSurfaceTypeBlend[VertexIndex];

			bool bNeedCompute = false;
			bool bIsComplete = true;
			for (const FVoxelSurfaceTypeBlendLayer& Layer : SurfaceTypeBlend.GetLayers())
			{
				if (Layer.Type.GetClass() == FVoxelSurfaceType::EClass::SurfaceTypeAsset)
				{
					continue;
				}
				bIsComplete = false;

				if (SurfaceTypeToSurfaceTypeBlend.Contains(Layer.Type))
				{
					bNeedCompute = true;
					continue;
				}

				SurfaceTypeToVerticesToCompute.FindOrAdd(Layer.Type).Add(VertexIndex);
			}

			if (bIsComplete)
			{
				checkVoxelSlow(!bNeedCompute);
				checkVoxelSlow(IncompleteVertices[VertexIndex]);
				IncompleteVertices[VertexIndex] = false;
				return;
			}

			bAllVerticesComplete = false;

			if (!bNeedCompute)
			{
				return;
			}

			TVoxelInlineArray<FVoxelSurfaceTypeBlendLayer, 32> LocalLayers;

			const auto AddLayer = [&](const FVoxelSurfaceTypeBlendLayer& Layer)
			{
				if (Layer.Weight == 0)
				{
					// Might happen when multiplying two small numbers
					return;
				}

				for (FVoxelSurfaceTypeBlendLayer& OtherLayer : LocalLayers)
				{
					if (OtherLayer.Type == Layer.Type)
					{
						OtherLayer.Weight += Layer.Weight;
						return;
					}
				}

				LocalLayers.Add(Layer);
			};

			for (const FVoxelSurfaceTypeBlendLayer& Layer : SurfaceTypeBlend.GetLayers())
			{
				if (Layer.Type.GetClass() == FVoxelSurfaceType::EClass::SurfaceTypeAsset)
				{
					AddLayer(Layer);
					continue;
				}

				const FVoxelSurfaceTypeBlend* NewSurfaceTypeBlend = SurfaceTypeToSurfaceTypeBlend.Find(Layer.Type);
				if (!NewSurfaceTypeBlend)
				{
					// Not ready yet, add as-is
					AddLayer(Layer);
					continue;
				}

				for (FVoxelSurfaceTypeBlendLayer NewLayer : NewSurfaceTypeBlend->GetLayers())
				{
					NewLayer.Weight *= Layer.Weight;
					AddLayer(NewLayer);
				}
			}

			SurfaceTypeBlend.InitializeFromLayers(LocalLayers);
		});
	}
}