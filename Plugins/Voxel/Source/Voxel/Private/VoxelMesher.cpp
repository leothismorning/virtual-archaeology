// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMesher.h"
#include "VoxelMesh.h"
#include "VoxelQuery.h"
#include "VoxelGraphPositionParameter.h"
#include "Surface/VoxelSurfaceTypeTable.h"
#include "Surface/VoxelSurfaceTypeBlendBuffer.h"
#include "Surface/VoxelSmartSurfaceTypeUtilities.h"
#include "Buffer/VoxelBaseBuffers.h"
#include "MegaMaterial/VoxelMegaMaterialProxy.h"

VOXEL_CONSOLE_VARIABLE(
	VOXEL_API, bool, GVoxelMesherShowSteps, false,
	"voxel.mesher.ShowSteps",
	"Show vertex projection steps");

VOXEL_CONSOLE_VARIABLE(
	VOXEL_API, bool, GVoxelMesherDisableProjection, false,
	"voxel.mesher.DisableProjection",
	"Disable verted projection",
	Voxel::RefreshAll);

VOXEL_CONSOLE_VARIABLE(
	VOXEL_API, bool, GVoxelMesherShowProcessedChunks, false,
	"voxel.mesher.ShowProcessedChunks",
	"Show status of chunks processed by the mesher",
	Voxel::Void,
	[]
	{
		if (!GVoxelMesherShowProcessedChunks)
		{
			return;
		}

		GEngine->AddOnScreenDebugMessage(
			FVoxelUtilities::MurmurHash(&GVoxelMesherShowProcessedChunks) ^ FVoxelUtilities::MurmurHash(0),
			2 * FApp::GetDeltaTime(),
			FColor::Red,
			"Computed height & volume");

		GEngine->AddOnScreenDebugMessage(
			FVoxelUtilities::MurmurHash(&GVoxelMesherShowProcessedChunks) ^ FVoxelUtilities::MurmurHash(1),
			2 * FApp::GetDeltaTime(),
			FColor::Yellow,
			"Reused height, computed volume");

		GEngine->AddOnScreenDebugMessage(
			FVoxelUtilities::MurmurHash(&GVoxelMesherShowProcessedChunks) ^ FVoxelUtilities::MurmurHash(2),
			2 * FApp::GetDeltaTime(),
			FColor::Blue,
			"Computed height and skipped chunk");
	});

VOXEL_CONSOLE_VARIABLE(
	VOXEL_API, bool, GVoxelMesherShowSkippedChunks, false,
	"voxel.mesher.ShowSkippedChunks",
	"Show chunks entirely skipped by the mesher");

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelMesher::FVoxelMesher(
	FVoxelLayers& Layers,
	FVoxelSurfaceTypeTable& SurfaceTypeTable,
	FVoxelDependencyCollector& DependencyCollector,
	const FVoxelWeakStackLayer& WeakLayer,
	const int32 ChunkLOD,
	const FInt64Vector& ChunkOffset,
	const int32 VoxelSize,
	const int32 ChunkSize,
	const FTransform& LocalToWorld,
	const FVoxelMegaMaterialProxy& MegaMaterialProxy,
	const FVoxelFloatMetadataRef BlockinessMetadata,
	const FVoxelMesherSettings& MesherSettings,
	const bool bExportDistances)
	: Layers(Layers)
	, SurfaceTypeTable(SurfaceTypeTable)
	, DependencyCollector(DependencyCollector)
	, WeakLayer(WeakLayer)
	, ChunkLOD(ChunkLOD)
	, ChunkOffset(ChunkOffset)
	, VoxelSize(VoxelSize)
	, ChunkSize(ChunkSize)
	, LocalToWorld(LocalToWorld)
	, MegaMaterialProxy(MegaMaterialProxy)
	, BlockinessMetadata(BlockinessMetadata)
	, MesherSettings(MesherSettings)
	, bExportDistances(bExportDistances)
	// We need edges to have accurate mesh normals & for Lumen
	, DataSize(ChunkSize + 4)
{
	VOXEL_FUNCTION_COUNTER();

	const int32 EstimatedNumCells = 4 * FMath::Square(ChunkSize + 2);

	CellToVertexIndex.Reserve(EstimatedNumCells);
	Edges.Reserve(3 * EstimatedNumCells);

	Vertices.Reserve(EstimatedNumCells);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<FVoxelMesh> FVoxelMesher::CreateMesh()
{
	VOXEL_FUNCTION_COUNTER();

	const FVector Start = FVector(ChunkOffset - (1 << ChunkLOD)) * VoxelSize;
	const float Step = float(1 << ChunkLOD) * VoxelSize;
	const FVoxelBox Bounds = FVoxelBox(Start, Start + FVector(DataSize) * Step);

	const FVoxelQuery Query(
		ChunkLOD,
		Layers,
		SurfaceTypeTable,
		DependencyCollector);

	if (!Query.HasStamps(WeakLayer, Bounds))
	{
		if (VOXEL_DEBUG)
		{
			const FVoxelFloatBuffer NewDistances = Query.SampleVolumeLayer(
				WeakLayer,
				Start,
				FIntVector(DataSize),
				Step);

			const FFloatInterval MinMax = FVoxelUtilities::GetMinMaxSafe(NewDistances.View());
			ensure(!MinMax.Contains(0));
		}

		if (GVoxelMesherShowSkippedChunks)
		{
			FVoxelDebugDrawer()
			.LifeTime(0.5f)
			.Color(FLinearColor::Green)
			.Thickness(2.f)
			.DrawBox(Bounds, LocalToWorld);
		}

		return MakeShared<FVoxelMesh>(
			ChunkLOD,
			ChunkOffset,
			ChunkSize,
			FVoxelUtilities::NaNf());
	}

	bool bCachedHeights = false;

	INLINE_LAMBDA
	{
		if (!Cache)
		{
			return;
		}

		const TVoxelOptional<FVoxelWeakStackLayer> HeightLayer = Query.GetFirstHeightLayer(WeakLayer);
		if (!HeightLayer)
		{
			return;
		}

		if (!Cache->bIsSet)
		{
			VOXEL_SCOPE_COUNTER("Cache heights");

			bCachedHeights = true;

			Cache->bIsSet = true;
			Cache->Start = FVector2D(Start);
			Cache->DataSize = DataSize;
			Cache->Step = Step;
			Cache->HeightLayer = HeightLayer.GetValue();
			Cache->DependencyCollector = MakeShared<FVoxelDependencyCollector>(STATIC_FNAME("FVoxelMesher HeightCache"));

			const FVoxelQuery HeightQuery(
				ChunkLOD,
				Layers,
				SurfaceTypeTable,
				*Cache->DependencyCollector);

			Cache->Heights = HeightQuery.SampleHeightLayer(
				HeightLayer.GetValue(),
				FVector2D(Start),
				FIntPoint(DataSize),
				Step);

			Cache->HeightRange = FVoxelUtilities::GetMinMaxSafe(Cache->Heights.View());
		}

		ensure(Cache->Start == FVector2D(Start));
		ensure(Cache->DataSize == DataSize);
		ensure(Cache->Step == Step);
		ensure(Cache->HeightLayer == HeightLayer.GetValue());
	};

	const bool bCanSkip = INLINE_LAMBDA
	{
		if (!Cache)
		{
			return false;
		}

		if (Query.HasVolumeStamps(WeakLayer, Bounds))
		{
			return false;
		}

		if (!Cache->HeightRange.IsValid())
		{
#if VOXEL_DEBUG
			for (const float Height : Cache->Heights)
			{
				ensure(FVoxelUtilities::IsNaN(Height));
			}
#endif

			return true;
		}

		return !FVoxelInterval(Cache->HeightRange).Intersects(Bounds.GetZ());
	};

	if (bCanSkip)
	{
		// Still track dependencies
		Query.DependencyCollector.AddDependencies(*Cache->DependencyCollector);

		if (GVoxelMesherShowProcessedChunks &&
			bCachedHeights)
		{
			FVoxelDebugDrawer()
			.LifeTime(0.5f)
			.Color(FLinearColor::Blue)
			.Thickness(2.f)
			.DrawBox(Bounds, LocalToWorld);
		}

		if (GVoxelMesherShowSkippedChunks)
		{
			FVoxelDebugDrawer()
			.LifeTime(0.5f)
			.Color(FLinearColor::Green)
			.Thickness(2.f)
			.DrawBox(Bounds, LocalToWorld);
		}

		return MakeShared<FVoxelMesh>(
			ChunkLOD,
			ChunkOffset,
			ChunkSize,
			FVoxelUtilities::NaNf());
	}

	if (GVoxelMesherShowProcessedChunks)
	{
		FVoxelDebugDrawer()
		.LifeTime(0.5f)
		.Color(Cache && !bCachedHeights ? FColor::Yellow : FColor::Red)
		.Thickness(5.f)
		.DrawBox(Bounds, LocalToWorld);
	}

	FVoxelQueryCache QueryCache;
	if (Cache)
	{
		QueryCache.HeightLayerToEntry.Add_EnsureNew(Cache->HeightLayer, FVoxelQueryCache::FEntry
		{
			Cache->Heights.View(),
			Cache->DependencyCollector
		});
	}

	DistancesBuffer = Query.SampleVolumeLayer(
		WeakLayer,
		Start,
		FIntVector(DataSize),
		Step,
		QueryCache);

	Distances = DistancesBuffer.View();

	const FFloatInterval MinMax = FVoxelUtilities::GetMinMaxSafe(Distances);
	if (!MinMax.Contains(0))
	{
		MinAbsDistance = FMath::Min(FMath::Abs(MinMax.Min), FMath::Abs(MinMax.Max));

		return Finalize();
	}

	MinAbsDistance = FVoxelUtilities::GetAbsMinSafe(Distances);

	GenerateVertices();
	GenerateTriangles();
	ProjectVertices();

	return Finalize();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMesher::GenerateVertices()
{
	VOXEL_FUNCTION_COUNTER();
	using namespace Voxel;

	for (int32 Z = -1; Z < ChunkSize + 2; Z++)
	{
		for (int32 Y = -1; Y < ChunkSize + 2; Y++)
		{
			const int32 BaseIndex = FVoxelUtilities::Get3DIndex<int32>(DataSize, 1, 1 + Y, 1 + Z);
			for (int32 X = -1; X < ChunkSize + 2; X++)
			{
				const int32 IndexOffset = BaseIndex + X;

#define INDEX(A, B, C) IndexOffset + A + B * DataSize + C * DataSize * DataSize

				checkVoxelSlow(INDEX(0, 0, 0) == FVoxelUtilities::Get3DIndex<int32>(DataSize, 1 + X + 0, 1 + Y + 0, 1 + Z + 0));
				checkVoxelSlow(INDEX(1, 0, 0) == FVoxelUtilities::Get3DIndex<int32>(DataSize, 1 + X + 1, 1 + Y + 0, 1 + Z + 0));
				checkVoxelSlow(INDEX(0, 1, 0) == FVoxelUtilities::Get3DIndex<int32>(DataSize, 1 + X + 0, 1 + Y + 1, 1 + Z + 0));
				checkVoxelSlow(INDEX(1, 1, 0) == FVoxelUtilities::Get3DIndex<int32>(DataSize, 1 + X + 1, 1 + Y + 1, 1 + Z + 0));
				checkVoxelSlow(INDEX(0, 0, 1) == FVoxelUtilities::Get3DIndex<int32>(DataSize, 1 + X + 0, 1 + Y + 0, 1 + Z + 1));
				checkVoxelSlow(INDEX(1, 0, 1) == FVoxelUtilities::Get3DIndex<int32>(DataSize, 1 + X + 1, 1 + Y + 0, 1 + Z + 1));
				checkVoxelSlow(INDEX(0, 1, 1) == FVoxelUtilities::Get3DIndex<int32>(DataSize, 1 + X + 0, 1 + Y + 1, 1 + Z + 1));
				checkVoxelSlow(INDEX(1, 1, 1) == FVoxelUtilities::Get3DIndex<int32>(DataSize, 1 + X + 1, 1 + Y + 1, 1 + Z + 1));

				const float Distance0 = Distances[INDEX(0, 0, 0)];
				const float Distance1 = Distances[INDEX(1, 0, 0)];
				const float Distance2 = Distances[INDEX(0, 1, 0)];
				const float Distance3 = Distances[INDEX(1, 1, 0)];
				const float Distance4 = Distances[INDEX(0, 0, 1)];
				const float Distance5 = Distances[INDEX(1, 0, 1)];
				const float Distance6 = Distances[INDEX(0, 1, 1)];
				const float Distance7 = Distances[INDEX(1, 1, 1)];

#undef INDEX

				if (Distance0 >= 0)
				{
					if (Distance1 >= 0 &&
						Distance2 >= 0 &&
						Distance3 >= 0 &&
						Distance4 >= 0 &&
						Distance5 >= 0 &&
						Distance6 >= 0 &&
						Distance7 >= 0)
					{
						continue;
					}
				}
				else
				{
					if (Distance1 < 0 &&
						Distance2 < 0 &&
						Distance3 < 0 &&
						Distance4 < 0 &&
						Distance5 < 0 &&
						Distance6 < 0 &&
						Distance7 < 0)
					{
						continue;
					}
				}

				if (FVoxelUtilities::IsNaN(Distance0) ||
					FVoxelUtilities::IsNaN(Distance1) ||
					FVoxelUtilities::IsNaN(Distance2) ||
					FVoxelUtilities::IsNaN(Distance3) ||
					FVoxelUtilities::IsNaN(Distance4) ||
					FVoxelUtilities::IsNaN(Distance5) ||
					FVoxelUtilities::IsNaN(Distance6) ||
					FVoxelUtilities::IsNaN(Distance7))
				{
					continue;
				}

				const TVoxelStaticArray<float, 8> CellDistances =
				{
					Distance0,
					Distance1,
					Distance2,
					Distance3,
					Distance4,
					Distance5,
					Distance6,
					Distance7,
				};

				int32 NumVertices = 0;
				FVector3f VertexSum = FVector3f(ForceInit);

				for (int32 EdgeIndex = 0; EdgeIndex < 12; EdgeIndex++)
				{
					const int32 Direction = EdgeIndex / 4;
					const int32 VertexIndex = EdgeIndex % 4;

					const int32 IndexA = INLINE_LAMBDA
					{
						switch (Direction)
						{
						default: VOXEL_ASSUME(false);
						case 0: return bool(VertexIndex & 0x0) + 2 * bool(VertexIndex & 0x1) + 4 * bool(VertexIndex & 0x2);
						case 1: return bool(VertexIndex & 0x1) + 2 * bool(VertexIndex & 0x0) + 4 * bool(VertexIndex & 0x2);
						case 2: return bool(VertexIndex & 0x1) + 2 * bool(VertexIndex & 0x2) + 4 * bool(VertexIndex & 0x0);
						}
					};
					const int32 IndexB = IndexA + (1 << Direction);

					const float DistanceA = CellDistances[IndexA];
					const float DistanceB = CellDistances[IndexB];

					if ((DistanceA >= 0) == (DistanceB >= 0))
					{
						continue;
					}

					if (IndexA == 0)
					{
						checkVoxelSlow(FVoxelUtilities::IsValidINT16(X));
						checkVoxelSlow(FVoxelUtilities::IsValidINT16(Y));
						checkVoxelSlow(FVoxelUtilities::IsValidINT16(Z));

						FEdge Edge;
						Edge.X = X;
						Edge.Y = Y;
						Edge.Z = Z;
						Edge.Direction = Direction;
						Edge.Sign = DistanceA < 0;

						checkVoxelSlow(!Edges.Contains(Edge));
						Edges.Add(Edge);
					}

					FVector3f Vertex = FVector3f(
						IndexA & 0x1 ? 1.f : 0.f,
						IndexA & 0x2 ? 1.f : 0.f,
						IndexA & 0x4 ? 1.f : 0.f);

					const float Alpha = DistanceA / (DistanceA - DistanceB);
					ensureVoxelSlow(0.f <= Alpha && Alpha <= 1.f);
					Vertex[Direction] = Alpha;

					NumVertices++;
					VertexSum += Vertex;
				}

				const FVector3f Alpha = VertexSum / NumVertices;
				ensureVoxelSlow(-KINDA_SMALL_NUMBER < Alpha.X && Alpha.X < 1.f + KINDA_SMALL_NUMBER);
				ensureVoxelSlow(-KINDA_SMALL_NUMBER < Alpha.Y && Alpha.Y < 1.f + KINDA_SMALL_NUMBER);
				ensureVoxelSlow(-KINDA_SMALL_NUMBER < Alpha.Z && Alpha.Z < 1.f + KINDA_SMALL_NUMBER);

				const FVector3f Vertex = FVector3f(X, Y, Z) + Alpha;

				const int32 VertexIndex = Vertices.Add(Vertex);
				ensureVoxelSlow(VertexIndex == VertexCells.Add(FCell(X, Y, Z)));

				CellToVertexIndex.Add_CheckNew(FCell(X, Y, Z), VertexIndex);
			}
		}
	}
}

void FVoxelMesher::GenerateTriangles()
{
	VOXEL_FUNCTION_COUNTER();

	Indices.Reserve(6 * Edges.Num());

	for (const FEdge& Edge : Edges)
	{
		const int32 AxisX = (Edge.Direction + 1) % 3;
		const int32 AxisY = (Edge.Direction + 2) % 3;

		FIntVector BasePosition(Edge.X, Edge.Y, Edge.Z);
		BasePosition[AxisX]--;
		BasePosition[AxisY]--;

		const auto FindVertex = [&](const int32 X, const int32 Y)
		{
			FIntVector Position = BasePosition;
			Position[AxisX] += X;
			Position[AxisY] += Y;

			const int32* VertexIndex = CellToVertexIndex.Find(FCell(Position.X, Position.Y, Position.Z));
			if (!VertexIndex)
			{
				// NaN or edge
				return -1;
			}

			return *VertexIndex;
		};

		const int32 Index00 = FindVertex(0, 0);
		const int32 Index11 = FindVertex(1, 1);

		if (Index00 == -1 ||
			Index11 == -1)
		{
			continue;
		}

		const FVector3f Vertex00 = Vertices[Index00];
		const FVector3f Vertex11 = Vertices[Index11];

		const int32 Index01 = FindVertex(1, 0);
		const int32 Index10 = FindVertex(0, 1);

		// Always add triangles if BlockinessMetadata is set, as they might end up being extended by the blockiness
		// TODO Better logic here? Still clean up the mesh at the end?

		if (Index01 != -1)
		{
			const FVector3f Vertex01 = Vertices[Index01];

			if (BlockinessMetadata.IsValid() ||
				FVoxelUtilities::IsTriangleValid(Vertex00, Vertex11, Vertex01))
			{
				if (Edge.Sign)
				{
					Indices.Add_EnsureNoGrow(Index00);
					Indices.Add_EnsureNoGrow(Index11);
					Indices.Add_EnsureNoGrow(Index01);
				}
				else
				{
					Indices.Add_EnsureNoGrow(Index01);
					Indices.Add_EnsureNoGrow(Index11);
					Indices.Add_EnsureNoGrow(Index00);
				}
			}
		}

		if (Index10 != -1)
		{
			const FVector3f Vertex10 = Vertices[Index10];

			if (BlockinessMetadata.IsValid() ||
				FVoxelUtilities::IsTriangleValid(Vertex00, Vertex10, Vertex11))
			{
				if (Edge.Sign)
				{
					Indices.Add_EnsureNoGrow(Index00);
					Indices.Add_EnsureNoGrow(Index10);
					Indices.Add_EnsureNoGrow(Index11);
				}
				else
				{
					Indices.Add_EnsureNoGrow(Index11);
					Indices.Add_EnsureNoGrow(Index10);
					Indices.Add_EnsureNoGrow(Index00);
				}
			}
		}
	}
}

void FVoxelMesher::ProjectVertices()
{
	VOXEL_FUNCTION_COUNTER_NUM(Vertices.Num());

	if (!MesherSettings.bEnableVertexProjection ||
		GVoxelMesherDisableProjection)
	{
		return;
	}

	FVoxelBitArray VerticesToProject;
	VerticesToProject.SetNum(Vertices.Num(), true);

	for (int32 Iteration = 0; Iteration < MesherSettings.MaxSteps; Iteration++)
	{
		const float Step = 1 << ChunkLOD;
		const float GradientStep = Step / 2.f;
		const float HalfGradientStep = GradientStep / 4.f;

		const int32 NumVertices = VerticesToProject.CountSetBits();
		if (NumVertices == 0)
		{
			break;
		}

		VOXEL_SCOPE_COUNTER_FORMAT("Iteration %d NumVertices=%d", Iteration, NumVertices);

		FVoxelDoubleVectorBuffer Positions;
		{
			VOXEL_SCOPE_COUNTER("Write positions");

			Positions.Allocate(7 * NumVertices);

			const TVoxelArrayView<double> PositionsX = Positions.X.View();
			const TVoxelArrayView<double> PositionsY = Positions.Y.View();
			const TVoxelArrayView<double> PositionsZ = Positions.Z.View();

			int32 WriteIndex = 0;
			VerticesToProject.ForAllSetBits([&](const int32 ReadIndex)
			{
				FVector Position = FVector(ChunkOffset) + FVector(Vertices[ReadIndex]) * Step;
				Position *= VoxelSize;

				PositionsX[7 * WriteIndex + 0] = Position.X - HalfGradientStep;
				PositionsX[7 * WriteIndex + 1] = Position.X + HalfGradientStep;
				PositionsX[7 * WriteIndex + 2] = Position.X;
				PositionsX[7 * WriteIndex + 3] = Position.X;
				PositionsX[7 * WriteIndex + 4] = Position.X;
				PositionsX[7 * WriteIndex + 5] = Position.X;

				PositionsY[7 * WriteIndex + 0] = Position.Y;
				PositionsY[7 * WriteIndex + 1] = Position.Y;
				PositionsY[7 * WriteIndex + 2] = Position.Y - HalfGradientStep;
				PositionsY[7 * WriteIndex + 3] = Position.Y + HalfGradientStep;
				PositionsY[7 * WriteIndex + 4] = Position.Y;
				PositionsY[7 * WriteIndex + 5] = Position.Y;

				PositionsZ[7 * WriteIndex + 0] = Position.Z;
				PositionsZ[7 * WriteIndex + 1] = Position.Z;
				PositionsZ[7 * WriteIndex + 2] = Position.Z;
				PositionsZ[7 * WriteIndex + 3] = Position.Z;
				PositionsZ[7 * WriteIndex + 4] = Position.Z - HalfGradientStep;
				PositionsZ[7 * WriteIndex + 5] = Position.Z + HalfGradientStep;

				PositionsX[7 * WriteIndex + 6] = Position.X;
				PositionsY[7 * WriteIndex + 6] = Position.Y;
				PositionsZ[7 * WriteIndex + 6] = Position.Z;

				WriteIndex++;
			});
			checkVoxelSlow(WriteIndex == NumVertices);
		}

		const FVoxelQuery Query(
			ChunkLOD,
			Layers,
			SurfaceTypeTable,
			DependencyCollector);

		const FVoxelFloatBuffer QueriedDistances = Query.SampleVolumeLayer(WeakLayer, Positions);

		int32 ReadIndex = 0;
		VerticesToProject.ForAllSetBits([&](const int32 WriteIndex)
		{
			const float DistanceMinX = QueriedDistances[7 * ReadIndex + 0];
			const float DistanceMaxX = QueriedDistances[7 * ReadIndex + 1];
			const float DistanceMinY = QueriedDistances[7 * ReadIndex + 2];
			const float DistanceMaxY = QueriedDistances[7 * ReadIndex + 3];
			const float DistanceMinZ = QueriedDistances[7 * ReadIndex + 4];
			const float DistanceMaxZ = QueriedDistances[7 * ReadIndex + 5];
			const float Distance = QueriedDistances[7 * ReadIndex + 6];
			ReadIndex++;

			if (!FVoxelUtilities::IsFinite(DistanceMinX) ||
				!FVoxelUtilities::IsFinite(DistanceMaxX) ||
				!FVoxelUtilities::IsFinite(DistanceMinY) ||
				!FVoxelUtilities::IsFinite(DistanceMaxY) ||
				!FVoxelUtilities::IsFinite(DistanceMinZ) ||
				!FVoxelUtilities::IsFinite(DistanceMaxZ) ||
				!FVoxelUtilities::IsFinite(Distance))
			{
				checkVoxelSlow(VerticesToProject[WriteIndex]);
				VerticesToProject[WriteIndex] = false;
				return;
			}

			double DeltaLength = Distance / Step / VoxelSize;
			DeltaLength = FMath::Clamp(DeltaLength, -MesherSettings.Speed, MesherSettings.Speed);

			if (FMath::Abs(DeltaLength) < 0.01f)
			{
				checkVoxelSlow(VerticesToProject[WriteIndex]);
				VerticesToProject[WriteIndex] = false;
				return;
			}

			const FVector3f Gradient =
				FVector3f(
					DistanceMaxX - DistanceMinX,
					DistanceMaxY - DistanceMinY,
					DistanceMaxZ - DistanceMinZ) / GradientStep;

			const FVector3f Delta = -Gradient.GetSafeNormal() * DeltaLength;
			FVector3f& Vertex = Vertices[WriteIndex];

			if (GVoxelMesherShowSteps)
			{
				FVector OldPosition = FVector(ChunkOffset) + FVector(Vertex) * Step;
				FVector NewPosition = FVector(ChunkOffset) + FVector(Vertex + Delta) * Step;

				OldPosition *= VoxelSize;
				NewPosition *= VoxelSize;

				OldPosition = LocalToWorld.TransformPosition(OldPosition);
				NewPosition = LocalToWorld.TransformPosition(NewPosition);

				FVoxelDebugDrawer()
				.DrawLine(OldPosition, NewPosition)
				.Color(FMath::Lerp(FLinearColor(FColor(FVoxelUtilities::MurmurHash32(WriteIndex))), FLinearColor::Black, Iteration / 10.f))
				.Thickness(0.5f + FMath::Clamp((3.f - Iteration) / 3.f, 0.f, 1.f))
				.LifeTime(10.f);
			}

			Vertex += Delta;

			const FCell VertexCell = VertexCells[WriteIndex];
			const FVector3f Center = FVector3f(VertexCell.X, VertexCell.Y, VertexCell.Z) + 0.5f;

			Vertex = FVoxelUtilities::Clamp(Vertex, Center - MesherSettings.MaxOffset, Center + MesherSettings.MaxOffset);
		});
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMesher::FPermutation::Update()
{
	VOXEL_FUNCTION_COUNTER();

	NewNumVertices = ValidVertices.CountSetBits();

	FVoxelUtilities::SetNumFast(OldToNewVertices, ValidVertices.Num());
	FVoxelUtilities::SetAll(OldToNewVertices, -1);

	int32 WriteIndex = 0;
	ValidVertices.ForAllSetBits([&](const int32 ReadIndex)
	{
		OldToNewVertices[ReadIndex] = WriteIndex++;
	});
	checkVoxelSlow(WriteIndex == NewNumVertices);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelMesher::FPermutation FVoxelMesher::GetPermutation() const
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelBitArray ValidVertices;
	ValidVertices.SetNum(Vertices.Num(), true);

	if (bRemoveEdges)
	{
		// We need edges to have accurate mesh normals & for Lumen

		for (int32 Index = 0; Index < Vertices.Num(); Index++)
		{
			const FCell Cell = VertexCells[Index];

			if (Cell.X < 0 ||
				Cell.Y < 0 ||
				Cell.Z < 0 ||
				Cell.X > ChunkSize ||
				Cell.Y > ChunkSize ||
				Cell.Z > ChunkSize)
			{
				ValidVertices[Index] = false;
			}
		}
	}

	{
		FVoxelBitArray UsedVertices;
		UsedVertices.SetNum(Vertices.Num(), false);

		checkVoxelSlow(Indices.Num() % 3 == 0);
		for (int32 Index = 0; Index < Indices.Num() / 3; Index++)
		{
			const int32 IndexA = Indices[3 * Index + 0];
			const int32 IndexB = Indices[3 * Index + 1];
			const int32 IndexC = Indices[3 * Index + 2];

			if (!ValidVertices[IndexA] ||
				!ValidVertices[IndexB] ||
				!ValidVertices[IndexC])
			{
				continue;
			}

			UsedVertices[IndexA] = true;
			UsedVertices[IndexB] = true;
			UsedVertices[IndexC] = true;
		}

		ValidVertices.BitwiseAnd(UsedVertices);
	}

	FPermutation Result;
	Result.ValidVertices = MoveTemp(ValidVertices);
	Result.Update();
	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelArray<int32> FVoxelMesher::ComputeIndices(const FPermutation& Permutation) const
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelArray<int32> NewIndices;
	NewIndices.Reserve(Indices.Num());

	checkVoxelSlow(Indices.Num() % 3 == 0);
	for (int32 Index = 0; Index < Indices.Num() / 3; Index++)
	{
		const int32 IndexA = Indices[3 * Index + 0];
		const int32 IndexB = Indices[3 * Index + 1];
		const int32 IndexC = Indices[3 * Index + 2];

		const int32 NewIndexA = Permutation.OldToNewVertices[IndexA];
		const int32 NewIndexB = Permutation.OldToNewVertices[IndexB];
		const int32 NewIndexC = Permutation.OldToNewVertices[IndexC];

		if (NewIndexA == -1 ||
			NewIndexB == -1 ||
			NewIndexC == -1)
		{
			continue;
		}

		const FCell VertexA = VertexCells[IndexA];
		const FCell VertexB = VertexCells[IndexB];
		const FCell VertexC = VertexCells[IndexC];

		const bool bVertexAOutside =
			VertexA.X >= ChunkSize ||
			VertexA.Y >= ChunkSize ||
			VertexA.Z >= ChunkSize;

		const bool bVertexBOutside =
			VertexB.X >= ChunkSize ||
			VertexB.Y >= ChunkSize ||
			VertexB.Z >= ChunkSize;

		const bool bVertexCOutside =
			VertexC.X >= ChunkSize ||
			VertexC.Y >= ChunkSize ||
			VertexC.Z >= ChunkSize;

		if (bVertexAOutside &&
			bVertexBOutside &&
			bVertexCOutside)
		{
			continue;
		}

		NewIndices.Add_EnsureNoGrow(NewIndexA);
		NewIndices.Add_EnsureNoGrow(NewIndexB);
		NewIndices.Add_EnsureNoGrow(NewIndexC);
	}

	return NewIndices;
}

TVoxelArray<FVector3f> FVoxelMesher::ComputeVertices(const FPermutation& Permutation) const
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelArray<FVector3f> NewVertices;
	NewVertices.Reserve(Permutation.NewNumVertices);

	Permutation.ValidVertices.ForAllSetBits([&](const int32 Index)
	{
		NewVertices.Add_EnsureNoGrow(Vertices[Index]);
	});

	return NewVertices;
}

FVoxelVectorBuffer FVoxelMesher::ComputeNormals(const FPermutation& Permutation) const
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelArray<FVector3f> Normals;
	FVoxelUtilities::SetNumZeroed(Normals, Vertices.Num());

	checkVoxelSlow(Indices.Num() % 3 == 0);
	for (int32 Index = 0; Index < Indices.Num() / 3; Index++)
	{
		const int32 IndexA = Indices[3 * Index + 0];
		const int32 IndexB = Indices[3 * Index + 1];
		const int32 IndexC = Indices[3 * Index + 2];

		const FVector3f Normal = FVoxelUtilities::GetTriangleNormal(
			Vertices[IndexA],
			Vertices[IndexB],
			Vertices[IndexC]);

		Normals[IndexA] += Normal;
		Normals[IndexB] += Normal;
		Normals[IndexC] += Normal;
	}

	FVoxelVectorBuffer Result;
	Result.Allocate(Permutation.NewNumVertices);

	int32 WriteIndex = 0;
	Permutation.ValidVertices.ForAllSetBits([&](const int32 Index)
	{
		Result.Set(WriteIndex++, Normals[Index].GetSafeNormal());
	});
	checkVoxelSlow(WriteIndex == Result.Num());

	return Result;
}

TVoxelArray<FVoxelMesh::FCell> FVoxelMesher::ComputeCells(const FPermutation& Permutation) const
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelArray<FVoxelMesh::FCell> Result;
	Result.Reserve(Permutation.NewNumVertices);

	Permutation.ValidVertices.ForAllSetBits([&](const int32 Index)
	{
		const FCell Cell = VertexCells[Index];
		Result.Add_EnsureNoGrow({ Cell.X, Cell.Y, Cell.Z });
	});

	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelMesh::FLOD FVoxelMesher::ComputeLOD(
	const FPermutation& Permutation,
	const int32 RelativeLOD) const
{
	VOXEL_SCOPE_COUNTER_FORMAT("FVoxelMesher::ComputeLOD RelativeLOD=%d", RelativeLOD);
	checkVoxelSlow(RelativeLOD >= 1);

	struct FParentCell
	{
		FIntVector Position;
		TVoxelStaticArray<int32, 8> DistanceIndices{ ForceInit };
		TVoxelArray<int32> VertexIndices;
	};
	TVoxelArray<FParentCell> ParentCells;
	ParentCells.Reserve(ChunkSize * 8);
	{
		TVoxelMap<FIntVector, int32> ParentCellToIndex;
		ParentCellToIndex.Reserve(ChunkSize * 8);

		const int32 Step = 1 << (RelativeLOD - 1);

		for (int32 OldVertexIndex = 0; OldVertexIndex < VertexCells.Num(); OldVertexIndex++)
		{
			const FCell Cell = VertexCells[OldVertexIndex];

			const bool bIsOnEdge =
				Cell.X < Step ||
				Cell.Y < Step ||
				Cell.Z < Step ||
				Cell.X > ChunkSize - Step ||
				Cell.Y > ChunkSize - Step ||
				Cell.Z > ChunkSize - Step;

			if (!bIsOnEdge)
			{
				continue;
			}

			const int32 VertexIndex = Permutation.OldToNewVertices[OldVertexIndex];
			if (VertexIndex == -1)
			{
				continue;
			}

			const FIntVector ParentCellPosition = FIntVector
			{
				FVoxelUtilities::DivideFloor(int32(Cell.X), 1 << RelativeLOD),
				FVoxelUtilities::DivideFloor(int32(Cell.Y), 1 << RelativeLOD),
				FVoxelUtilities::DivideFloor(int32(Cell.Z), 1 << RelativeLOD)
			};

			const uint32 Hash = ParentCellToIndex.HashValue(ParentCellPosition);

			if (const int32* ParentCellIndex = ParentCellToIndex.FindHashed(Hash, ParentCellPosition))
			{
				checkVoxelSlow(ParentCells[*ParentCellIndex].Position == ParentCellPosition);
				ParentCells[*ParentCellIndex].VertexIndices.Add(VertexIndex);
				continue;
			}

			ParentCellToIndex.AddHashed_CheckNew(Hash, ParentCellPosition, ParentCells.Num());

			FParentCell& ParentCell = ParentCells.Emplace_GetRef();
			ParentCell.VertexIndices.Reserve(FMath::Square(1 << RelativeLOD));

			ParentCell.Position = ParentCellPosition;
			ParentCell.VertexIndices.Add(VertexIndex);
		}
	}

	if (ParentCells.Num() == 0)
	{
		return {};
	}

	TVoxelMap<FIntVector, int32> PositionToIndex;
	PositionToIndex.Reserve(8 * ParentCells.Num());

	FVoxelDoubleVectorBuffer Positions;
	Positions.Allocate(8 * ParentCells.Num());

	const int32 Step = 1 << (ChunkLOD + RelativeLOD);

	int32 WriteIndex = 0;
	for (FParentCell& ParentCell : ParentCells)
	{
		for (int32 Corner = 0; Corner < 8; Corner++)
		{
			const FIntVector Position
			{
				ParentCell.Position.X + bool(Corner & 0x1),
				ParentCell.Position.Y + bool(Corner & 0x2),
				ParentCell.Position.Z + bool(Corner & 0x4)
			};

			const uint32 Hash = PositionToIndex.HashValue(Position);

			if (const int32* Index = PositionToIndex.FindHashed(Hash, Position))
			{
				ParentCell.DistanceIndices[Corner] = *Index;
				continue;
			}

			const FVector FinalPosition = FVector(ChunkOffset + FInt64Vector3(Position) * Step) * VoxelSize;

			Positions.Set(WriteIndex, FinalPosition);

			ParentCell.DistanceIndices[Corner] = WriteIndex;
			PositionToIndex.AddHashed_CheckNew_EnsureNoGrow(Hash, Position, WriteIndex);

			WriteIndex++;
		}
	}

	Positions.ShrinkTo(WriteIndex);

	const FVoxelQuery Query(
		ChunkLOD + RelativeLOD,
		Layers,
		SurfaceTypeTable,
		DependencyCollector);

	FVoxelFloatBuffer Blockiness;
	TVoxelMap<FVoxelMetadataRef, TSharedRef<FVoxelFloatBuffer>> MetadataToBuffer;

	if (BlockinessMetadata)
	{
		Blockiness.AllocateZeroed(Positions.Num());
		MetadataToBuffer.Add_EnsureNew(BlockinessMetadata, MakeSharedCopy(Blockiness));
	}

	const FVoxelFloatBuffer ParentDistances = Query.SampleVolumeLayer(
		WeakLayer,
		Positions,
		{},
		MetadataToBuffer);

	FVoxelMesh::FLOD LOD;
	FVoxelUtilities::SetNumFast(LOD.VertexIndexToDisplacedVertexIndex, Permutation.NewNumVertices);
	FVoxelUtilities::SetAll(LOD.VertexIndexToDisplacedVertexIndex, -1);
	LOD.DisplacedVertices.Reserve(ParentCells.Num());

	for (const FParentCell& ParentCell : ParentCells)
	{
		TVoxelStaticArray<float, 8> CellDistances{ NoInit };
		for (int32 Corner = 0; Corner < 8; Corner++)
		{
			CellDistances[Corner] = ParentDistances[ParentCell.DistanceIndices[Corner]];
		}

		if (FVoxelUtilities::IsNaN(CellDistances[0]) ||
			FVoxelUtilities::IsNaN(CellDistances[1]) ||
			FVoxelUtilities::IsNaN(CellDistances[2]) ||
			FVoxelUtilities::IsNaN(CellDistances[3]) ||
			FVoxelUtilities::IsNaN(CellDistances[4]) ||
			FVoxelUtilities::IsNaN(CellDistances[5]) ||
			FVoxelUtilities::IsNaN(CellDistances[6]) ||
			FVoxelUtilities::IsNaN(CellDistances[7]))
		{
			//ensureVoxelSlow(false);
			continue;
		}

		if ((CellDistances[0] >= 0) == (CellDistances[1] >= 0) &&
			(CellDistances[0] >= 0) == (CellDistances[2] >= 0) &&
			(CellDistances[0] >= 0) == (CellDistances[3] >= 0) &&
			(CellDistances[0] >= 0) == (CellDistances[4] >= 0) &&
			(CellDistances[0] >= 0) == (CellDistances[5] >= 0) &&
			(CellDistances[0] >= 0) == (CellDistances[6] >= 0) &&
			(CellDistances[0] >= 0) == (CellDistances[7] >= 0))
		{
			//ensureVoxelSlow(false);
			continue;
		}

		int32 NumVertices = 0;
		FVector3f VertexSum = FVector3f(ForceInit);

		for (int32 EdgeIndex = 0; EdgeIndex < 12; EdgeIndex++)
		{
			const int32 Direction = EdgeIndex / 4;
			const int32 VertexIndex = EdgeIndex % 4;

			const int32 IndexA = INLINE_LAMBDA
			{
				switch (Direction)
				{
				default: VOXEL_ASSUME(false);
				case 0: return bool(VertexIndex & 0x0) + 2 * bool(VertexIndex & 0x1) + 4 * bool(VertexIndex & 0x2);
				case 1: return bool(VertexIndex & 0x1) + 2 * bool(VertexIndex & 0x0) + 4 * bool(VertexIndex & 0x2);
				case 2: return bool(VertexIndex & 0x1) + 2 * bool(VertexIndex & 0x2) + 4 * bool(VertexIndex & 0x0);
				}
			};
			const int32 IndexB = IndexA + (1 << Direction);

			const float DistanceA = CellDistances[IndexA];
			const float DistanceB = CellDistances[IndexB];

			if ((DistanceA >= 0) == (DistanceB >= 0))
			{
				continue;
			}

			FVector3f Vertex = FVector3f(
				IndexA & 0x1 ? 1.f : 0.f,
				IndexA & 0x2 ? 1.f : 0.f,
				IndexA & 0x4 ? 1.f : 0.f);

			const float Alpha = DistanceA / (DistanceA - DistanceB);
			ensureVoxelSlow(0.f <= Alpha && Alpha <= 1.f);
			Vertex[Direction] = Alpha;

			NumVertices++;
			VertexSum += Vertex;
		}

		const FVector3f Alpha = VertexSum / NumVertices;
		ensureVoxelSlow(-KINDA_SMALL_NUMBER < Alpha.X && Alpha.X < 1.f + KINDA_SMALL_NUMBER);
		ensureVoxelSlow(-KINDA_SMALL_NUMBER < Alpha.Y && Alpha.Y < 1.f + KINDA_SMALL_NUMBER);
		ensureVoxelSlow(-KINDA_SMALL_NUMBER < Alpha.Z && Alpha.Z < 1.f + KINDA_SMALL_NUMBER);

		FVector3f Vertex = FVector3f(ParentCell.Position) + Alpha;

		if (BlockinessMetadata)
		{
			float FinalBlockiness = 0.f;
			for (int32 Corner = 0; Corner < 8; Corner++)
			{
				FinalBlockiness += Blockiness[ParentCell.DistanceIndices[Corner]];
			}
			FinalBlockiness /= 8.f;

			Vertex = FMath::Lerp(
				Vertex,
				FVector3f(ParentCell.Position) + FVector3f(0.5f),
				FMath::Clamp(FinalBlockiness, 0.f, 1.f));
		}

		Vertex *= (1 << RelativeLOD);

 		const int32 DisplacedVertexIndex = LOD.DisplacedVertices.Add_EnsureNoGrow(Vertex);

		for (const int32 VertexIndex : ParentCell.VertexIndices)
		{
			LOD.VertexIndexToDisplacedVertexIndex[VertexIndex] = DisplacedVertexIndex;
		}
	}

	return LOD;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<FVoxelMesh> FVoxelMesher::Finalize()
{
	VOXEL_FUNCTION_COUNTER();
	checkVoxelSlow(MinAbsDistance == FVoxelUtilities::GetAbsMinSafe(Distances));

	if (Indices.Num() == 0)
	{
		return MakeShared<FVoxelMesh>(
			ChunkLOD,
			ChunkOffset,
			ChunkSize,
			MinAbsDistance);
	}

	FPermutation Permutation = GetPermutation();

	if (Permutation.NewNumVertices == 0)
	{
		return MakeShared<FVoxelMesh>(
			ChunkLOD,
			ChunkOffset,
			ChunkSize,
			MinAbsDistance);
	}

	FVoxelDoubleVectorBuffer QueryPositions;
	{
		VOXEL_SCOPE_COUNTER_NUM("QueryPositions", Permutation.NewNumVertices);

		QueryPositions.Allocate(Permutation.NewNumVertices);

		const float Step = 1 << ChunkLOD;

		int32 WriteIndex = 0;

		Permutation.ValidVertices.ForAllSetBits([&](const int32 Index)
		{
			FVector Vertex = FVector(ChunkOffset) + FVector(Vertices[Index]) * Step;
			Vertex *= VoxelSize;

			QueryPositions.Set(WriteIndex++, Vertex);
		});

		checkVoxelSlow(WriteIndex == Permutation.NewNumVertices);
	}

	FVoxelSurfaceTypeBlendBuffer SurfaceTypes;
	SurfaceTypes.AllocateZeroed(QueryPositions.Num());
	{
		VOXEL_SCOPE_COUNTER_NUM("SurfaceTypes", Permutation.NewNumVertices);

		FVoxelFloatBuffer Blockiness;
		TVoxelMap<FVoxelMetadataRef, TSharedRef<FVoxelFloatBuffer>> MetadataToBuffer;

		if (BlockinessMetadata)
		{
			Blockiness.AllocateZeroed(QueryPositions.Num());
			MetadataToBuffer.Add_EnsureNew(BlockinessMetadata, MakeSharedCopy(Blockiness));
		}

		const FVoxelQuery Query(
			ChunkLOD,
			Layers,
			SurfaceTypeTable,
			DependencyCollector);

		(void)Query.SampleVolumeLayer(
			WeakLayer,
			QueryPositions,
			SurfaceTypes.View(),
			MetadataToBuffer);

		if (BlockinessMetadata)
		{
			VOXEL_SCOPE_COUNTER("Blockiness");

			for (const auto& It : CellToVertexIndex)
			{
				const int32 OldIndex = It.Value;
				const int32 NewIndex = Permutation.OldToNewVertices[OldIndex];
				if (NewIndex == -1)
				{
					continue;
				}

				const FVector3f NewPosition = FVector3f(It.Key.X, It.Key.Y, It.Key.Z) + 0.5f;

				Vertices[OldIndex] = FMath::Lerp(Vertices[OldIndex], NewPosition, FMath::Clamp(Blockiness[NewIndex], 0.f, 1.f));
			}
		}
	}

	FVoxelVectorBuffer Normals = ComputeNormals(Permutation);

	FVoxelSmartSurfaceTypeUtilities::Resolve(
		ChunkLOD,
		WeakLayer,
		DependencyCollector,
		Layers,
		SurfaceTypeTable,
		QueryPositions,
		Normals,
		SurfaceTypes.View());

	TVoxelInlineSet<FVoxelSurfaceType, 32> UsedSurfaceTypesSet;
	TVoxelArray<FVoxelSurfaceType> UsedSurfaceTypes;
	{
		VOXEL_SCOPE_COUNTER("UsedSurfaces");

		for (const FVoxelSurfaceTypeBlend& SurfaceType : SurfaceTypes)
		{
			for (const FVoxelSurfaceTypeBlendLayer& Layer : SurfaceType.GetLayers())
			{
				UsedSurfaceTypesSet.Add(Layer.Type);
			};
		}

		UsedSurfaceTypes = UsedSurfaceTypesSet.Array();
		UsedSurfaceTypes.Sort();
	}

	check(SurfaceTypes.Num() == Normals.Num());

	DependencyCollector.AddDependency(*SurfaceTypeTable.InvisibleSurfaceTypesDependency);

	if (SurfaceTypeTable.InvisibleSurfaceTypes.Num() > 0 &&
		UsedSurfaceTypesSet.Contains(SurfaceTypeTable.InvisibleSurfaceTypes))
	{
		VOXEL_SCOPE_COUNTER("InvisibleSurfaceTypes");

		int32 ReadIndex = 0;
		int32 WriteIndex = 0;
		Permutation.ValidVertices.ForAllSetBits([&](const int32 Index)
		{
			const FVoxelSurfaceTypeBlend& SurfaceType = SurfaceTypes[ReadIndex];

			if (SurfaceTypeTable.InvisibleSurfaceTypes.Contains(SurfaceType.GetTopLayer().Type))
			{
				Permutation.ValidVertices[Index] = false;
				ReadIndex++;
				return;
			}

			if (WriteIndex != ReadIndex)
			{
				QueryPositions.Set(WriteIndex, QueryPositions[ReadIndex]);
				SurfaceTypes.Set(WriteIndex, SurfaceType);
				Normals.Set(WriteIndex, Normals[ReadIndex]);
			}

			WriteIndex++;
			ReadIndex++;
		});

		if (WriteIndex != SurfaceTypes.Num())
		{
			QueryPositions.ShrinkTo(WriteIndex);
			SurfaceTypes.ShrinkTo(WriteIndex);
			Normals.ShrinkTo(WriteIndex);
			Permutation.Update();
		}
	}

	if (Permutation.NewNumVertices == 0)
	{
		return MakeShared<FVoxelMesh>(
			ChunkLOD,
			ChunkOffset,
			ChunkSize,
			MinAbsDistance);
	}

	TVoxelMap<FVoxelMetadataRef, TSharedRef<FVoxelBuffer>> MetadataToBuffer;
	INLINE_LAMBDA
	{
		if (!bQueryMetadata)
		{
			return;
		}

		VOXEL_SCOPE_COUNTER("Metadata");

		TVoxelSet<FVoxelMetadataRef> MetadataRefs;
		MetadataRefs.Reserve(32);

		for (const FVoxelSurfaceType SurfaceType : UsedSurfaceTypes)
		{
			const FVoxelMaterialRenderIndex RenderIndex = MegaMaterialProxy.GetRenderIndex(SurfaceType);
			MetadataRefs.Append(MegaMaterialProxy.GetUsedMetadatas(RenderIndex));
		}

		if (MetadataRefs.Num() == 0)
		{
			return;
		}

		for (const FVoxelMetadataRef& MetadataRef : MetadataRefs)
		{
			MetadataToBuffer.Add_EnsureNew(
				MetadataRef,
				MetadataRef.MakeDefaultBuffer(QueryPositions.Num()));
		}

		const FVoxelQuery Query(
			ChunkLOD,
			Layers,
			SurfaceTypeTable,
			DependencyCollector);

		(void)Query.SampleVolumeLayer(
			WeakLayer,
			QueryPositions,
			{},
			MetadataToBuffer);
	};

	TVoxelArray<FVoxelMesh::FLOD> LODs;
	for (int32 RelativeLOD = 1; RelativeLOD <= MaxRelativeLOD; RelativeLOD++)
	{
		LODs.Add(ComputeLOD(Permutation, RelativeLOD));
	}

	const TSharedRef<FVoxelMesh> Mesh = MakeShared<FVoxelMesh>(
		ChunkLOD,
		ChunkOffset,
		ChunkSize,
		MinAbsDistance,
		MoveTemp(UsedSurfaceTypes),
		MoveTemp(MetadataToBuffer),
		ComputeIndices(Permutation),
		ComputeVertices(Permutation),
		FVoxelUtilities::MakeOctahedrons(
			Normals.X.View(),
			Normals.Y.View(),
			Normals.Z.View()),
		SurfaceTypes.View().Array(),
		ComputeCells(Permutation),
		MoveTemp(LODs));

	if (bExportDistances)
	{
		Mesh->DistancesOffset = -1;
		Mesh->DistancesSize = DataSize;
		Mesh->Distances = Distances.Array();
	}

	Mesh->UpdateStats();
	return Mesh;
}