// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMesh.h"
#include "VoxelStackLayer.h"
#include "VoxelMesherSettings.h"
#include "VoxelFloatMetadataRef.h"
#include "Buffer/VoxelFloatBuffers.h"

class FVoxelLayers;
class FVoxelSurfaceTypeTable;
class FVoxelMegaMaterialProxy;

struct FVoxelMesherCache
{
	bool bIsSet = false;
	FVector2D Start = FVector2D(ForceInit);
	int32 DataSize = 0;
	float Step = 0;
	FVoxelWeakStackLayer HeightLayer;
	TSharedPtr<FVoxelDependencyCollector> DependencyCollector;

	FVoxelFloatBuffer Heights;
	FFloatInterval HeightRange;
};

class FVoxelMesher
{
public:
	static constexpr int32 MaxRelativeLOD = 3;

	FVoxelLayers& Layers;
	FVoxelSurfaceTypeTable& SurfaceTypeTable;
	FVoxelDependencyCollector& DependencyCollector;
	const FVoxelWeakStackLayer WeakLayer;
	const int32 ChunkLOD;
	const FInt64Vector ChunkOffset;
	const int32 VoxelSize;
	const int32 ChunkSize;
	const FTransform LocalToWorld;
	const FVoxelMegaMaterialProxy& MegaMaterialProxy;
	const FVoxelFloatMetadataRef BlockinessMetadata;
	const FVoxelMesherSettings MesherSettings;
	const bool bExportDistances;

	const int32 DataSize;

	bool bRemoveEdges = false;
	bool bQueryMetadata = false;
	TSharedPtr<FVoxelMesherCache> Cache;

	explicit FVoxelMesher(
		FVoxelLayers& Layers,
		FVoxelSurfaceTypeTable& SurfaceTypeTable,
		FVoxelDependencyCollector& DependencyCollector,
		const FVoxelWeakStackLayer& WeakLayer,
		int32 ChunkLOD,
		const FInt64Vector& ChunkOffset,
		int32 VoxelSize,
		int32 ChunkSize,
		const FTransform& LocalToWorld,
		const FVoxelMegaMaterialProxy& MegaMaterialProxy,
		FVoxelFloatMetadataRef BlockinessMetadata,
		const FVoxelMesherSettings& MesherSettings,
		bool bExportDistances);

	TSharedRef<FVoxelMesh> CreateMesh();

private:
	FVoxelFloatBuffer DistancesBuffer;
	TConstVoxelArrayView<float> Distances;
	float MinAbsDistance = -1;

	struct alignas(8) FCell
	{
		int16 X;
		int16 Y;
		int16 Z;
		int16 Padding;

		FCell() = default;
		FCell(
			const int32 X,
			const int32 Y,
			const int32 Z)
			: X(X)
			, Y(Y)
			, Z(Z)
			, Padding(0)
		{
			checkVoxelSlow(FVoxelUtilities::IsValidINT16(X));
			checkVoxelSlow(FVoxelUtilities::IsValidINT16(Y));
			checkVoxelSlow(FVoxelUtilities::IsValidINT16(Z));
		}

		FORCEINLINE bool operator==(const FCell& Other) const
		{
			checkVoxelSlow(Padding == 0);
			checkVoxelSlow(Other.Padding == 0);

			return ReinterpretCastRef<uint64>(*this) == ReinterpretCastRef<uint64>(Other);
		}
		FORCEINLINE friend uint32 GetTypeHash(const FCell& Cell)
		{
			checkVoxelSlow(Cell.Padding == 0);
			return FVoxelUtilities::MurmurHash64(ReinterpretCastRef<uint64>(Cell));
		}
	};
	TVoxelMap<FCell, int32> CellToVertexIndex;

	struct alignas(8) FEdge
	{
		int16 X;
		int16 Y;
		int16 Z;
		uint8 Direction;
		uint8 Sign;

		FORCEINLINE FEdge()
		{
			ReinterpretCastRef<uint64>(*this) = 0;
		}

		FORCEINLINE bool operator==(const FEdge& Other) const
		{
			return ReinterpretCastRef<uint64>(*this) == ReinterpretCastRef<uint64>(Other);
		}
	};
	TVoxelArray<FEdge> Edges;

	TVoxelArray<int32> Indices;
	TVoxelArray<FCell> VertexCells;
	TVoxelArray<FVector3f> Vertices;

private:
	void GenerateVertices();
	void GenerateTriangles();
	void ProjectVertices();

private:
	struct FPermutation
	{
		FVoxelBitArray ValidVertices;
		TVoxelArray<int32> OldToNewVertices;
		int32 NewNumVertices = 0;

		void Update();
	};
	FPermutation GetPermutation() const;

private:
	TVoxelArray<int32> ComputeIndices(const FPermutation& Permutation) const;
	TVoxelArray<FVector3f> ComputeVertices(const FPermutation& Permutation) const;
	FVoxelVectorBuffer ComputeNormals(const FPermutation& Permutation) const;
	TVoxelArray<FVoxelMesh::FCell> ComputeCells(const FPermutation& Permutation) const;

	FVoxelMesh::FLOD ComputeLOD(
		const FPermutation& Permutation,
		int32 RelativeLOD) const;

private:
	TSharedRef<FVoxelMesh> Finalize();
};