// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Height/VoxelHeightSculptEditor.h"
#include "Sculpt/Height/VoxelHeightSculptInnerData.h"
#include "Sculpt/Height/VoxelHeightSculptUtilities.h"
#include "Sculpt/Height/VoxelHeightChunkTree.h"
#include "Sculpt/Height/VoxelHeightHeightChunk.h"
#include "Sculpt/Height/VoxelHeightMetadataChunk.h"
#include "Sculpt/Height/VoxelHeightSurfaceTypeChunk.h"

FVoxelBox2D FVoxelHeightSculptEditor::DoWork(FVoxelHeightSculptInnerData& SculptData)
{
	VOXEL_FUNCTION_COUNTER();
	check(Cache->SculptToWorld == SculptToWorld);

	Modifier->GetUsage(
		bWritesHeights,
		bWritesSurfaceTypes,
		MetadataRefsToWrite);

	BoundsToSculpt = FVoxelIntBox2D::FromFloatBox_WithPadding(
		Modifier->GetBounds()
		.TransformBy(SculptToWorld.Inverse())
		.Extend(2));

	// Add a border of size 1 for jump flood
	BoundsToSculpt = BoundsToSculpt.MakeMultipleOfBigger(ChunkSize).Extend(ChunkSize);

	Size = BoundsToSculpt.Size();

	ChunkKeyOffset = BoundsToSculpt.Min / ChunkSize;
	RelativeChunkKeyBounds = FVoxelIntBox2D(0, Size).DivideExact(ChunkSize);
	RelativeChunkKeyBoundsToEdit = FVoxelIntBox2D(ChunkSize, Size - ChunkSize).DivideExact(ChunkSize);

	if (!ensure(int64(Size.X) * int64(Size.Y) < MAX_int32))
	{
		return {};
	}

	{
		VOXEL_SCOPE_COUNTER("Query Heights");

		const TVoxelHeightChunkTree<FVoxelHeightHeightChunk>& Tree = *SculptData.HeightChunkTree;

		FVoxelUtilities::SetNumFast(Heights, Size.X * Size.Y);
		FVoxelUtilities::SetNumFast(PreviousHeights, Size.X * Size.Y);

		FVoxelParallelTaskScope Scope;

		RelativeChunkKeyBounds.Iterate([&](const FIntPoint& RelativeChunkKey)
		{
			Scope.AddTask([&, RelativeChunkKey]
			{
				const FIntPoint ChunkKey = ChunkKeyOffset + RelativeChunkKey;

				const TSharedRef<const FVoxelHeightSculptPreviousChunk> PreviousChunk = Cache->ComputeChunk(
					*Layers,
					*SurfaceTypeTable,
					WeakLayer,
					ChunkKey);

				const FVoxelHeightHeightChunk* SculptChunk = Tree.FindChunk(ChunkKey);

				for (int32 IndexY = 0; IndexY < ChunkSize; IndexY++)
				{
					for (int32 IndexX = 0; IndexX < ChunkSize; IndexX++)
					{
						const int32 IndexInChunk = FVoxelUtilities::Get2DIndex<int32>(
							ChunkSize,
							IndexX,
							IndexY);

						const int32 IndexInSculpt = FVoxelUtilities::Get2DIndex<int32>(
							Size,
							RelativeChunkKey.X * ChunkSize + IndexX,
							RelativeChunkKey.Y * ChunkSize + IndexY);

						float Height = PreviousChunk->Heights[IndexInChunk];

						PreviousHeights[IndexInSculpt] = Height;

						if (SculptChunk)
						{
							const float SculptHeight = SculptChunk->Heights[IndexInChunk];

							if (!FVoxelUtilities::IsNaN(SculptHeight))
							{
								if (bRelativeHeight)
								{
									Height += SculptHeight * ScaleZ + OffsetZ;
								}
								else
								{
									Height = SculptHeight * ScaleZ + OffsetZ;
								}
							}
						}

						Heights[IndexInSculpt] = Height;
					}
				}
			});
		});
	}

	ApplyModifier(SculptData);

	extern bool GVoxelSculptShowChunkBounds;

	if (GVoxelSculptShowChunkBounds)
	{
		FVoxelDebugDrawer()
		.DrawBox(
			BoundsToSculpt.ToVoxelBox2D().ToBox3D_Infinite().Extend(0.01),
			SculptToWorld.To3DMatrix())
		.Color(FLinearColor::Blue)
		.LifeTime(5)
		.Thickness(10.f);
	}

	return BoundsToSculpt.ToVoxelBox2D();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelHeightSculptEditor::ApplyModifier(FVoxelHeightSculptInnerData& SculptData)
{
	VOXEL_FUNCTION_COUNTER();

	TSharedPtr<FVoxelHeightModifier::FSurfaceTypes> SurfaceTypes;
	TVoxelMap<FVoxelMetadataRef, TSharedPtr<FVoxelHeightModifier::FMetadata>> MetadataRefToMetadata;

	if (bWritesSurfaceTypes ||
		MetadataRefsToWrite.Num() > 0)
	{
		FVoxelDoubleVector2DBuffer Positions;
		{
			VOXEL_SCOPE_COUNTER("Write positions");

			Positions.Allocate(Heights.Num());

			int32 WriteIndex = 0;
			for (int32 Y = 0; Y < Size.Y; Y++)
			{
				for (int32 X = 0; X < Size.X; X++)
				{
					const FVector2D Position = FVector2D(BoundsToSculpt.Min + FIntPoint(X, Y));

					Positions.Set(WriteIndex, Position);
					WriteIndex++;
				}
			}
			check(WriteIndex == Positions.Num());
		}

		if (bWritesSurfaceTypes)
		{
			VOXEL_SCOPE_COUNTER("Query surface types");

			SurfaceTypes = MakeShared<FVoxelHeightModifier::FSurfaceTypes>();

			SculptData.GetSurfaceTypes(
				Positions,
				SurfaceTypes->Alphas,
				SurfaceTypes->SurfaceTypes);
		}

		MetadataRefToMetadata.Reserve(MetadataRefsToWrite.Num());

		for (const FVoxelMetadataRef& MetadataRef : MetadataRefsToWrite)
		{
			VOXEL_SCOPE_COUNTER_FORMAT("Query metadata %s", *MetadataRef.GetFName().ToString());

			const TSharedRef<FVoxelHeightModifier::FMetadata> Metadata = MakeShared<FVoxelHeightModifier::FMetadata>();
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

		const FTransform2d IndexToWorld =
			FTransform2d(FVector2D(BoundsToSculpt.Min)) *
			SculptToWorld;

		Modifier->Apply(FVoxelHeightModifier::FData
		{
			Heights,
			Size,
			SurfaceTypes,
			MetadataRefToMetadata,
			RelativeChunkKeyBoundsToEdit.Scale(ChunkSize),
			IndexToWorld
		});
	}

	{
		VOXEL_SCOPE_COUNTER("Fixup heights");

		const FVoxelIntBox2D BoundsToEdit = RelativeChunkKeyBoundsToEdit.Scale(ChunkSize);

		const TVoxelArray<float> HeightsCopy = Heights;

		Voxel::ParallelFor(BoundsToEdit.GetY(), [&](const int32 IndexY)
		{
			for (int32 IndexX = BoundsToEdit.Min.X; IndexX < BoundsToEdit.Max.X; IndexX++)
			{
				const int32 Index = FVoxelUtilities::Get2DIndex<int32>(
					Size,
					IndexX,
					IndexY);

				float& Height = Heights[Index];

				if (FVoxelUtilities::IsNaN(Height))
				{
					continue;
				}

				if (bRelativeHeight)
				{
					const float PreviousHeight = PreviousHeights[Index];

					if (!FVoxelUtilities::IsNaN(PreviousHeight))
					{
						Height -= PreviousHeight;
					}
				}
				else
				{
					const bool bCanRemove = INLINE_LAMBDA
					{
						// Ensure we always have padding to make sure bilinear interpolation works

						for (int32 Y = -1; Y <= 1; Y++)
						{
							for (int32 X = -1; X <= 1; X++)
							{
								const int32 NeighborIndex = FVoxelUtilities::Get2DIndex<int32>(Size, IndexX + X, IndexY + Y);

								if (HeightsCopy[NeighborIndex] != PreviousHeights[NeighborIndex])
								{
									return false;
								}
							}
						}

						return true;
					};

					if (bCanRemove)
					{
						Height = FVoxelUtilities::NaNf();
					}
				}
			}
		});
	}

	if (bWritesHeights)
	{
		VOXEL_SCOPE_COUNTER("Heights");

		TVoxelHeightChunkTree<FVoxelHeightHeightChunk>& Tree = *SculptData.HeightChunkTree;

		FVoxelParallelTaskScope Scope;
		FVoxelCriticalSection CriticalSection;

		RelativeChunkKeyBoundsToEdit.Iterate([&](const FIntPoint& RelativeChunkKey)
		{
			Scope.AddTask([&, RelativeChunkKey]
			{
				const TVoxelRefCountPtr<FVoxelHeightHeightChunk> Chunk = FVoxelHeightSculptUtilities::CreateHeightChunk(
					RelativeChunkKey,
					Heights,
					Size,
					ScaleZ,
					OffsetZ);

				FFloatInterval Range(MAX_flt, -MAX_flt);
				if (Chunk)
				{
					Range = FVoxelUtilities::GetMinMaxSafe(Chunk->Heights);
				}

				VOXEL_SCOPE_LOCK(CriticalSection);

				SculptData.MinHeight = FMath::Min(SculptData.MinHeight, Range.Min);
				SculptData.MaxHeight = FMath::Max(SculptData.MaxHeight, Range.Max);

				Tree.SetChunk(ChunkKeyOffset + RelativeChunkKey, Chunk);
			});
		});
	}

	if (bWritesSurfaceTypes)
	{
		VOXEL_SCOPE_COUNTER("Write surface types");

		TVoxelHeightChunkTree<FVoxelHeightSurfaceTypeChunk>& Tree = *SculptData.SurfaceTypeChunkTree;

		FVoxelParallelTaskScope Scope;
		FVoxelCriticalSection CriticalSection;

		RelativeChunkKeyBoundsToEdit.Iterate([&](const FIntPoint& RelativeChunkKey)
		{
			Scope.AddTask([&, RelativeChunkKey]
			{
				const TVoxelRefCountPtr<FVoxelHeightSurfaceTypeChunk> Chunk = FVoxelHeightSculptUtilities::CreateSurfaceTypeChunk(
					RelativeChunkKey,
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
		const FVoxelHeightModifier::FMetadata& Metadata = *MetadataRefToMetadata[MetadataRef];

		TSharedPtr<TVoxelHeightChunkTree<FVoxelHeightMetadataChunk>>& Tree = SculptData.MetadataRefToChunkTree.FindOrAdd(MetadataRef);
		if (!Tree)
		{
			Tree = MakeShared<TVoxelHeightChunkTree<FVoxelHeightMetadataChunk>>();
		}

		FVoxelParallelTaskScope Scope;
		FVoxelCriticalSection CriticalSection;

		RelativeChunkKeyBoundsToEdit.Iterate([&](const FIntPoint& RelativeChunkKey)
		{
			Scope.AddTask([&, RelativeChunkKey]
			{
				const TVoxelRefCountPtr<FVoxelHeightMetadataChunk> Chunk = FVoxelHeightSculptUtilities::CreateMetadataChunk(
					RelativeChunkKey,
					Metadata.Alphas,
					*Metadata.Buffer,
					Size);

				VOXEL_SCOPE_LOCK(CriticalSection);

				Tree->SetChunk(ChunkKeyOffset + RelativeChunkKey, Chunk);
			});
		});
	}
}