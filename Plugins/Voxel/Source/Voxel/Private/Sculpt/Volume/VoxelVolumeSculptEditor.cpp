// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Volume/VoxelVolumeSculptEditor.h"
#include "Sculpt/Volume/VoxelVolumeSculptInnerData.h"
#include "Sculpt/Volume/VoxelVolumeSculptUtilities.h"
#include "Sculpt/Volume/VoxelVolumeChunkTree.h"
#include "Sculpt/Volume/VoxelVolumeDistanceChunk.h"
#include "Sculpt/Volume/VoxelVolumeMetadataChunk.h"
#include "Sculpt/Volume/VoxelVolumeSurfaceTypeChunk.h"
#include "Sculpt/Volume/VoxelVolumeFastDistanceChunk.h"

VOXEL_CONSOLE_VARIABLE(
	VOXEL_API, bool, GVoxelSculptShowChunkBounds, false,
	"voxel.sculpt.ShowChunkBounds",
	"Show chunks sculpted in editor");

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelBox FVoxelVolumeSculptEditor::DoWork(FVoxelVolumeSculptInnerData& SculptData)
{
	VOXEL_FUNCTION_COUNTER();
	check(Cache->SculptToWorld.Equals(SculptToWorld, 0.));

	Modifier->GetUsage(
		bWritesDistances,
		bWritesSurfaceTypes,
		MetadataRefsToWrite);

	BoundsToSculpt = FVoxelIntBox::FromFloatBox_WithPadding(
		Modifier->GetBounds()
		.TransformBy(SculptToWorld.Inverse())
		.Extend(2));

	// Add a border of size 1 for jump flood
	BoundsToSculpt = BoundsToSculpt.MakeMultipleOfBigger(ChunkSize).Extend(ChunkSize);

	Size = BoundsToSculpt.Size();

	ChunkKeyOffset = BoundsToSculpt.Min / ChunkSize;
	RelativeChunkKeyBounds = FVoxelIntBox(0, Size).DivideExact(ChunkSize);
	RelativeChunkKeyBoundsToEdit = FVoxelIntBox(ChunkSize, Size - ChunkSize).DivideExact(ChunkSize);

	if (!ensure(int64(Size.X) * int64(Size.Y) * int64(Size.Z) < MAX_int32))
	{
		return {};
	}

	if (SculptData.bUseFastDistances)
	{
		VOXEL_SCOPE_COUNTER("Query distances");
		ensure(!bEnableDiffing);

		const TVoxelVolumeChunkTree<FVoxelVolumeFastDistanceChunk>& Tree = *SculptData.DistanceChunkTree_LQ;

		FVoxelUtilities::SetNumFast(Distances, Size.X * Size.Y * Size.Z);
		FVoxelUtilities::SetNumFast(PreviousDistances, Size.X * Size.Y * Size.Z);

		FVoxelParallelTaskScope Scope;

		RelativeChunkKeyBounds.Iterate([&](const FIntVector& RelativeChunkKey)
		{
			Scope.AddTask([&, RelativeChunkKey]
			{
				const FIntVector ChunkKey = ChunkKeyOffset + RelativeChunkKey;

				const TSharedRef<const FVoxelVolumeSculptPreviousChunk> PreviousChunk = Cache->ComputeChunk(
					*Layers,
					*SurfaceTypeTable,
					WeakLayer,
					ChunkKey);

				const FVoxelVolumeFastDistanceChunk* SculptChunk = Tree.FindChunk(ChunkKey);

				for (int32 IndexZ = 0; IndexZ < ChunkSize; IndexZ++)
				{
					for (int32 IndexY = 0; IndexY < ChunkSize; IndexY++)
					{
						for (int32 IndexX = 0; IndexX < ChunkSize; IndexX++)
						{
							const int32 IndexInChunk = FVoxelUtilities::Get3DIndex<int32>(
								ChunkSize,
								IndexX,
								IndexY,
								IndexZ);

							const int32 IndexInSculpt = FVoxelUtilities::Get3DIndex<int32>(
								Size,
								RelativeChunkKey.X * ChunkSize + IndexX,
								RelativeChunkKey.Y * ChunkSize + IndexY,
								RelativeChunkKey.Z * ChunkSize + IndexZ);

							float Distance = PreviousChunk->Distances[IndexInChunk];

							if (BlendMode == EVoxelVolumeBlendMode::Intersect)
							{
								Distance = FVoxelUtilities::NaNf();
							}

							PreviousDistances[IndexInSculpt] = Distance;

							if (!SculptChunk)
							{
								Distances[IndexInSculpt] = Distance;
								continue;
							}

							Distances[IndexInSculpt] = SculptChunk->GetDistance(IndexInChunk) * VoxelSize;
						}
					}
				}
			});
		});
	}
	else
	{
		VOXEL_SCOPE_COUNTER("Query distances");

		const TVoxelVolumeChunkTree<FVoxelVolumeDistanceChunk>& Tree = *SculptData.DistanceChunkTree_HQ;

		FVoxelUtilities::SetNumFast(Distances, Size.X * Size.Y * Size.Z);
		FVoxelUtilities::SetNumFast(PreviousDistances, Size.X * Size.Y * Size.Z);

		FVoxelParallelTaskScope Scope;

		RelativeChunkKeyBounds.Iterate([&](const FIntVector& RelativeChunkKey)
		{
			Scope.AddTask([&, RelativeChunkKey]
			{
				const FIntVector ChunkKey = ChunkKeyOffset + RelativeChunkKey;

				const TSharedRef<const FVoxelVolumeSculptPreviousChunk> PreviousChunk = Cache->ComputeChunk(
					*Layers,
					*SurfaceTypeTable,
					WeakLayer,
					ChunkKey);

				const FVoxelVolumeDistanceChunk* SculptChunk = Tree.FindChunk(ChunkKey);

				for (int32 IndexZ = 0; IndexZ < ChunkSize; IndexZ++)
				{
					for (int32 IndexY = 0; IndexY < ChunkSize; IndexY++)
					{
						for (int32 IndexX = 0; IndexX < ChunkSize; IndexX++)
						{
							const int32 IndexInChunk = FVoxelUtilities::Get3DIndex<int32>(
								ChunkSize,
								IndexX,
								IndexY,
								IndexZ);

							const int32 IndexInSculpt = FVoxelUtilities::Get3DIndex<int32>(
								Size,
								RelativeChunkKey.X * ChunkSize + IndexX,
								RelativeChunkKey.Y * ChunkSize + IndexY,
								RelativeChunkKey.Z * ChunkSize + IndexZ);

							float Distance = PreviousChunk->Distances[IndexInChunk];

							if (BlendMode == EVoxelVolumeBlendMode::Intersect)
							{
								Distance = FVoxelUtilities::NaNf();
							}

							PreviousDistances[IndexInSculpt] = Distance;

							ON_SCOPE_EXIT
							{
								Distances[IndexInSculpt] = Distance;
							};

							if (!SculptChunk)
							{
								continue;
							}

							float AdditiveDistance = SculptChunk->AdditiveDistances[IndexInChunk];
							float SubtractiveDistance = SculptChunk->SubtractiveDistances[IndexInChunk];

							// AdditiveDistance will be NaN if we only removed
							// SubtractiveDistance will be NaN if we only added

							if (!FVoxelUtilities::IsNaN(AdditiveDistance))
							{
								AdditiveDistance = AdditiveDistance * VoxelSize;

								if (FVoxelUtilities::IsNaN(Distance))
								{
									Distance = AdditiveDistance;
								}
								else
								{
									Distance = FMath::Min(Distance, AdditiveDistance);
								}
							}

							if (!FVoxelUtilities::IsNaN(SubtractiveDistance))
							{
								SubtractiveDistance = -SubtractiveDistance * VoxelSize;

								if (FVoxelUtilities::IsNaN(Distance))
								{
									Distance = SubtractiveDistance;
								}
								else
								{
									Distance = FMath::Max(Distance, SubtractiveDistance);
								}
							}
						}
					}
				}
			});
		});
	}

	{
		VOXEL_SCOPE_COUNTER("PreviousDistances");

		const TVoxelArray<float> OldPreviousDistances = PreviousDistances;

		FVoxelUtilities::JumpFlood(Size, VoxelSize, PreviousDistances);

		// Jump flooding might fail if we're too deep underground
		for (int32 Index = 0; Index < PreviousDistances.Num(); Index++)
		{
			float& PreviousDistance = PreviousDistances[Index];

			if (FVoxelUtilities::IsNaN(PreviousDistance))
			{
				PreviousDistance = OldPreviousDistances[Index];
			}
		}
	}

	FVoxelUtilities::JumpFlood(
		Size,
		VoxelSize,
		Distances,
		ClosestX,
		ClosestY,
		ClosestZ);

	ApplyModifier(SculptData);
	Propagate(SculptData);

	if (GVoxelSculptShowChunkBounds)
	{
		FVoxelDebugDrawer()
		.DrawBox(
			BoundsToSculpt.ToVoxelBox().Extend(0.01),
			SculptToWorld)
		.Color(FLinearColor::Blue)
		.LifeTime(5)
		.Thickness(10.f);
	}

	return BoundsToSculpt.ToVoxelBox();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelVolumeSculptEditor::ApplyModifier(FVoxelVolumeSculptInnerData& SculptData)
{
	VOXEL_FUNCTION_COUNTER();

	TSharedPtr<FVoxelVolumeModifier::FSurfaceTypes> SurfaceTypes;
	TVoxelMap<FVoxelMetadataRef, TSharedPtr<FVoxelVolumeModifier::FMetadata>> MetadataRefToMetadata;

	if (bWritesSurfaceTypes ||
		MetadataRefsToWrite.Num() > 0)
	{
		FVoxelDoubleVectorBuffer Positions;
		{
			VOXEL_SCOPE_COUNTER("Write positions");

			Positions.Allocate(Distances.Num());

			int32 WriteIndex = 0;
			for (int32 Z = 0; Z < Size.Z; Z++)
			{
				for (int32 Y = 0; Y < Size.Y; Y++)
				{
					for (int32 X = 0; X < Size.X; X++)
					{
						const FVector Position = FVector(BoundsToSculpt.Min + FIntVector(X, Y, Z));

						Positions.Set(WriteIndex, Position);
						WriteIndex++;
					}
				}
			}
			check(WriteIndex == Positions.Num());
		}

		if (bWritesSurfaceTypes)
		{
			VOXEL_SCOPE_COUNTER("Query surface types");

			SurfaceTypes = MakeShared<FVoxelVolumeModifier::FSurfaceTypes>();

			SculptData.GetSurfaceTypes(
				Positions,
				SurfaceTypes->Alphas,
				SurfaceTypes->SurfaceTypes);
		}

		MetadataRefToMetadata.Reserve(MetadataRefsToWrite.Num());

		for (const FVoxelMetadataRef& MetadataRef : MetadataRefsToWrite)
		{
			VOXEL_SCOPE_COUNTER_FORMAT("Query metadata %s", *MetadataRef.GetFName().ToString());

			const TSharedRef<FVoxelVolumeModifier::FMetadata> Metadata = MakeShared<FVoxelVolumeModifier::FMetadata>();
			MetadataRefToMetadata.Add_EnsureNew(MetadataRef, Metadata);

			SculptData.GetMetadatas(
				MetadataRef,
				Positions,
				Metadata->Alphas,
				Metadata->Buffer);
		}
	}

	{
		VOXEL_SCOPE_COUNTER("Edit");

		const FMatrix IndexToWorld =
			FTranslationMatrix(FVector(BoundsToSculpt.Min)) *
			SculptToWorld;

		Modifier->Apply(FVoxelVolumeModifier::FData
		{
			Distances,
			Size,
			SurfaceTypes,
			MetadataRefToMetadata,
			RelativeChunkKeyBoundsToEdit.Scale(ChunkSize),
			IndexToWorld
		});
	}

	if (bWritesDistances)
	{
		VOXEL_SCOPE_COUNTER("Distances");

		FVoxelUtilities::JumpFlood(Size, VoxelSize, Distances);

		if (SculptData.bUseFastDistances)
		{
			TVoxelVolumeChunkTree<FVoxelVolumeFastDistanceChunk>& Tree = *SculptData.DistanceChunkTree_LQ;

			FVoxelParallelTaskScope Scope;
			FVoxelCriticalSection CriticalSection;

			RelativeChunkKeyBoundsToEdit.Iterate([&](const FIntVector& RelativeChunkKey)
			{
				Scope.AddTask([&, RelativeChunkKey]
				{
					const TVoxelRefCountPtr<FVoxelVolumeFastDistanceChunk> Chunk = FVoxelVolumeSculptUtilities::CreateFastDistanceChunk(
						RelativeChunkKey,
						Distances,
						Size,
						VoxelSize);

					VOXEL_SCOPE_LOCK(CriticalSection);

					Tree.SetChunk(ChunkKeyOffset + RelativeChunkKey, Chunk);
				});
			});
		}
		else if (bEnableDiffing)
		{
			TVoxelVolumeChunkTree<FVoxelVolumeDistanceChunk>& Tree = *SculptData.DistanceChunkTree_HQ;

			TVoxelArray<float> AdditiveDistances;
			TVoxelArray<float> SubtractiveDistances;
			FVoxelVolumeSculptUtilities::DiffDistances(
				Distances,
				PreviousDistances,
				Size,
				VoxelSize,
				AdditiveDistances,
				SubtractiveDistances);

			FVoxelParallelTaskScope Scope;
			FVoxelCriticalSection CriticalSection;

			RelativeChunkKeyBoundsToEdit.Iterate([&](const FIntVector& RelativeChunkKey)
			{
				Scope.AddTask([&, RelativeChunkKey]
				{
					const TVoxelRefCountPtr<FVoxelVolumeDistanceChunk> Chunk = FVoxelVolumeSculptUtilities::CreateDistanceChunk_Diffing(
						RelativeChunkKey,
						AdditiveDistances,
						SubtractiveDistances,
						Size,
						VoxelSize);

					VOXEL_SCOPE_LOCK(CriticalSection);

					Tree.SetChunk(ChunkKeyOffset + RelativeChunkKey, Chunk);
				});
			});
		}
		else
		{
			TVoxelVolumeChunkTree<FVoxelVolumeDistanceChunk>& Tree = *SculptData.DistanceChunkTree_HQ;

			FVoxelParallelTaskScope Scope;
			FVoxelCriticalSection CriticalSection;

			RelativeChunkKeyBoundsToEdit.Iterate([&](const FIntVector& RelativeChunkKey)
			{
				Scope.AddTask([&, RelativeChunkKey]
				{
					const TVoxelRefCountPtr<FVoxelVolumeDistanceChunk> Chunk = FVoxelVolumeSculptUtilities::CreateDistanceChunk_NoDiffing(
						RelativeChunkKey,
						Distances,
						Size,
						VoxelSize);

					VOXEL_SCOPE_LOCK(CriticalSection);

					Tree.SetChunk(ChunkKeyOffset + RelativeChunkKey, Chunk);
				});
			});
		}
	}

	if (bWritesSurfaceTypes)
	{
		VOXEL_SCOPE_COUNTER("Write surface types");

		TVoxelVolumeChunkTree<FVoxelVolumeSurfaceTypeChunk>& Tree = *SculptData.SurfaceTypeChunkTree;

		FVoxelParallelTaskScope Scope;
		FVoxelCriticalSection CriticalSection;

		RelativeChunkKeyBoundsToEdit.Iterate([&](const FIntVector& RelativeChunkKey)
		{
			Scope.AddTask([&, RelativeChunkKey]
			{
				const TVoxelRefCountPtr<FVoxelVolumeSurfaceTypeChunk> Chunk = FVoxelVolumeSculptUtilities::CreateSurfaceTypeChunk(
					RelativeChunkKey,
					{},
					SurfaceTypes->Alphas,
					SurfaceTypes->SurfaceTypes,
					Size);

				VOXEL_SCOPE_LOCK(CriticalSection);

				Tree.SetChunk(ChunkKeyOffset + RelativeChunkKey, Chunk);
			});
		});
	}

	for (const FVoxelMetadataRef& MetadataRef : MetadataRefsToWrite)
	{
		VOXEL_SCOPE_COUNTER_FORMAT("Write metadata %s", *MetadataRef.GetFName().ToString());
		const FVoxelVolumeModifier::FMetadata& Metadata = *MetadataRefToMetadata[MetadataRef];

		TSharedPtr<TVoxelVolumeChunkTree<FVoxelVolumeMetadataChunk>>& Tree = SculptData.MetadataRefToChunkTree.FindOrAdd(MetadataRef);
		if (!Tree)
		{
			Tree = MakeShared<TVoxelVolumeChunkTree<FVoxelVolumeMetadataChunk>>();
		}

		FVoxelParallelTaskScope Scope;
		FVoxelCriticalSection CriticalSection;

		RelativeChunkKeyBoundsToEdit.Iterate([&](const FIntVector& RelativeChunkKey)
		{
			Scope.AddTask([&, RelativeChunkKey]
			{
				const TVoxelRefCountPtr<FVoxelVolumeMetadataChunk> Chunk = FVoxelVolumeSculptUtilities::CreateMetadataChunk(
					RelativeChunkKey,
					{},
					Metadata.Alphas,
					*Metadata.Buffer,
					Size);

				VOXEL_SCOPE_LOCK(CriticalSection);

				Tree->SetChunk(ChunkKeyOffset + RelativeChunkKey, Chunk);
			});
		});
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelVolumeSculptEditor::Propagate(FVoxelVolumeSculptInnerData& SculptData)
{
	VOXEL_FUNCTION_COUNTER();

	if (bWritesSurfaceTypes ||
		MetadataRefsToWrite.Num() > 0)
	{
		return;
	}

	FVoxelDoubleVectorBuffer Positions;
	FVoxelInt32Buffer Indirection;
	{
		VOXEL_SCOPE_COUNTER("Write positions");

		Positions.Allocate(Distances.Num());
		Indirection.Allocate(Distances.Num());

		int32 WriteIndex = 0;
		for (int32 Index = 0; Index < Distances.Num(); Index++)
		{
			const FVector Closest = FVector(
				ClosestX[Index],
				ClosestY[Index],
				ClosestZ[Index]);

			if (FVoxelUtilities::IsNaN(Closest.X) ||
				FVoxelUtilities::IsNaN(Closest.Y) ||
				FVoxelUtilities::IsNaN(Closest.Z))
			{
				Indirection.Set(Index, -1);
				continue;
			}

			const FVector Position = FVector(BoundsToSculpt.Min) + Closest;

			Positions.Set(WriteIndex, Position);
			Indirection.Set(Index, WriteIndex);

			WriteIndex++;
		}

		Positions.ShrinkTo(WriteIndex);
	}

	{
		VOXEL_SCOPE_COUNTER("Propagate surface types")

		TVoxelArray<float> Alphas;
		TVoxelArray<FVoxelSurfaceTypeBlend> SurfaceTypes;

		SculptData.GetSurfaceTypes(
			Positions,
			Alphas,
			SurfaceTypes);

		TVoxelVolumeChunkTree<FVoxelVolumeSurfaceTypeChunk>& Tree = *SculptData.SurfaceTypeChunkTree;

		FVoxelParallelTaskScope Scope;
		FVoxelCriticalSection CriticalSection;

		RelativeChunkKeyBoundsToEdit.Iterate([&](const FIntVector& RelativeChunkKey)
		{
			Scope.AddTask([&, RelativeChunkKey]
			{
				const TVoxelRefCountPtr<FVoxelVolumeSurfaceTypeChunk> Chunk = FVoxelVolumeSculptUtilities::CreateSurfaceTypeChunk(
					RelativeChunkKey,
					Indirection.View(),
					Alphas,
					SurfaceTypes,
					Size);

				VOXEL_SCOPE_LOCK(CriticalSection);

				Tree.SetChunk(ChunkKeyOffset + RelativeChunkKey, Chunk);
			});
		});
	}

	for (const auto& It : SculptData.MetadataRefToChunkTree)
	{
		VOXEL_SCOPE_COUNTER_FORMAT("Propagate metadata %s", *It.Key.GetFName().ToString());

		TVoxelArray<float> Alphas;
		TSharedPtr<FVoxelBuffer> Metadata;

		SculptData.GetMetadatas(
			It.Key,
			Positions,
			Alphas,
			Metadata);

		FVoxelParallelTaskScope Scope;
		FVoxelCriticalSection CriticalSection;

		RelativeChunkKeyBoundsToEdit.Iterate([&](const FIntVector& RelativeChunkKey)
		{
			Scope.AddTask([&, RelativeChunkKey]
			{
				const TVoxelRefCountPtr<FVoxelVolumeMetadataChunk> Chunk = FVoxelVolumeSculptUtilities::CreateMetadataChunk(
					RelativeChunkKey,
					Indirection.View(),
					Alphas,
					*Metadata,
					Size);

				VOXEL_SCOPE_LOCK(CriticalSection);

				It.Value->SetChunk(ChunkKeyOffset + RelativeChunkKey, Chunk);
			});
		});
	}
}