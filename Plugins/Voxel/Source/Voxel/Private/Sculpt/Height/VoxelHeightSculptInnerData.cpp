// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Height/VoxelHeightSculptInnerData.h"
#include "Sculpt/Height/VoxelHeightChunkTree.h"
#include "Sculpt/Height/VoxelHeightHeightChunk.h"
#include "Sculpt/Height/VoxelHeightMetadataChunk.h"
#include "Sculpt/Height/VoxelHeightSurfaceTypeChunk.h"
#include "VoxelMetadata.h"
#include "Buffer/VoxelFloatBuffers.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

FVoxelHeightSculptInnerData::FVoxelHeightSculptInnerData()
	: HeightChunkTree(MakeShared<TVoxelHeightChunkTree<FVoxelHeightHeightChunk>>())
	, SurfaceTypeChunkTree(MakeShared<TVoxelHeightChunkTree<FVoxelHeightSurfaceTypeChunk>>())
{
}

void FVoxelHeightSculptInnerData::Serialize(FArchive& InAr)
{
	VOXEL_FUNCTION_COUNTER();

	FObjectAndNameAsStringProxyArchive StringAr(InAr, true);

	FArchive& Ar = InAr.GetArchiveName() == "FVoxelArchive" ? StringAr : InAr;

	ON_SCOPE_EXIT
	{
		MinHeight = MAX_flt;
		MaxHeight = -MAX_flt;

		for (const auto& It : HeightChunkTree->GetKeyToChunk())
		{
			const FFloatInterval Range = FVoxelUtilities::GetMinMaxSafe(It.Value->Heights);
			MinHeight = FMath::Min(MinHeight, Range.Min);
			MaxHeight = FMath::Max(MaxHeight, Range.Max);
		}
	};

	using FVersion = DECLARE_VOXEL_VERSION
	(
		FirstVersion,
		FirstRefactor
	);

	int32 Version = FVersion::LatestVersion;
	Ar << Version;

	if (Version < FVersion::FirstRefactor)
	{
		FIntPoint MinKey;
		FIntPoint MaxKey;
		Ar << MinKey;
		Ar << MaxKey;

		TVoxelArray<FIntPoint> ChunkKeys;
		Ar << ChunkKeys;

		for (const FIntPoint& ChunkKey : ChunkKeys)
		{
			const TVoxelRefCountPtr<FVoxelHeightHeightChunk> Chunk = new FVoxelHeightHeightChunk();
			Ar << Chunk->Heights;
			HeightChunkTree->SetChunk(ChunkKey, Chunk);
		}

		return;
	}

	HeightChunkTree->Serialize(Ar);
	SurfaceTypeChunkTree->Serialize(Ar);

	TVoxelArray<UVoxelMetadata*> Metadatas;
	if (Ar.IsSaving())
	{
		TVoxelArray<FVoxelMetadataRef> MetadataRefs = MetadataRefToChunkTree.KeyArray();
		Metadatas.Reserve(MetadataRefs.Num());

		for (const FVoxelMetadataRef& MetadataRef : MetadataRefs)
		{
			UVoxelMetadata* Metadata = MetadataRef.GetMetadata().Resolve();
			ensureVoxelSlow(Metadata);
			Metadatas.Add_EnsureNoGrow(Metadata);
		}
	}

	Ar << Metadatas;

	if (Ar.IsLoading())
	{
		MetadataRefToChunkTree.Reset();
		MetadataRefToChunkTree.Reserve(Metadatas.Num());

		for (UVoxelMetadata* Metadata : Metadatas)
		{
			const TSharedRef<TVoxelHeightChunkTree<FVoxelHeightMetadataChunk>> Tree = MakeShared<TVoxelHeightChunkTree<FVoxelHeightMetadataChunk>>();
			Tree->Serialize(Ar);

			if (!ensureVoxelSlow(Metadata))
			{
				continue;
			}

			MetadataRefToChunkTree.Add_EnsureNew(FVoxelMetadataRef(Metadata), Tree);
		}
	}
	else
	{
		for (const auto& It : MetadataRefToChunkTree)
		{
			It.Value->Serialize(Ar);
		}
	}
}

void FVoxelHeightSculptInnerData::CopyFrom(const FVoxelHeightSculptInnerData& Other)
{
	VOXEL_FUNCTION_COUNTER();

	HeightChunkTree->CopyFrom(*Other.HeightChunkTree);
	SurfaceTypeChunkTree->CopyFrom(*Other.SurfaceTypeChunkTree);

	MetadataRefToChunkTree.Reset();
	MetadataRefToChunkTree.Reserve(Other.MetadataRefToChunkTree.Num());

	for (const auto& It : Other.MetadataRefToChunkTree)
	{
		const TSharedRef<TVoxelHeightChunkTree<FVoxelHeightMetadataChunk>> Tree = MakeShared<TVoxelHeightChunkTree<FVoxelHeightMetadataChunk>>();
		Tree->CopyFrom(*It.Value);
		MetadataRefToChunkTree.Add_EnsureNew(It.Key, Tree);
	}

	MinHeight = Other.MinHeight;
	MaxHeight = Other.MaxHeight;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelHeightSculptInnerData::ApplyShape(
	const FVoxelHeightBulkQuery& Query,
	const FVoxelHeightTransform& StampToQuery,
	const float ScaleXY,
	const EVoxelHeightBlendMode BlendMode,
	const bool bApplyOnVoid,
	const bool bRelativeHeight) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Query.Num());

	const FVoxelBox2D Bounds = StampToQuery.InverseTransform(Query.GetBounds()).Scale(1. / ScaleXY);

	if (!HeightChunkTree->HasChunks(Bounds))
	{
		return;
	}

	const TVoxelHeightChunkTreeIterator<FVoxelHeightHeightChunk> Iterator = HeightChunkTree->CreateIterator(
		Query,
		StampToQuery,
		ScaleXY);

	ProcessHeights(
		Iterator,
		[&](
			const int32 Index,
			float Height)
		{
			if (FVoxelUtilities::IsNaN(Height))
			{
				return;
			}

			Height = StampToQuery.TransformHeight(Height, Query.GetPositionFromIndex(Index));

			float& QueryHeight = Query.Heights[Index];

			if (bRelativeHeight &&
				!FVoxelUtilities::IsNaN(Height) &&
				!FVoxelUtilities::IsNaN(QueryHeight))
			{
				Height += QueryHeight;
			}

			QueryHeight = FVoxelHeightHeightChunk::Blend(
				QueryHeight,
				Height,
				BlendMode,
				bApplyOnVoid);
		});
}

void FVoxelHeightSculptInnerData::ApplyShape(
	const FVoxelHeightSparseQuery& Query,
	const FVoxelHeightTransform& StampToQuery,
	const float ScaleXY,
	const EVoxelHeightBlendMode BlendMode,
	const bool bApplyOnVoid,
	const bool bRelativeHeight) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Query.Num());

	const FVoxelBox2D Bounds = StampToQuery.InverseTransform(Query.PositionBounds).Scale(1. / ScaleXY);

	if (!HeightChunkTree->HasChunks(Bounds))
	{
		return;
	}

	const TVoxelHeightChunkTreeIterator<FVoxelHeightHeightChunk> Iterator = HeightChunkTree->CreateIterator(
		Query,
		StampToQuery,
		ScaleXY);

	ProcessHeights(
		Iterator,
		[&](
			const int32 Index,
			float Height)
		{
			if (FVoxelUtilities::IsNaN(Height))
			{
				return;
			}

			const int32 IndirectIndex = Query.GetIndirectIndex(Index);

			Height = StampToQuery.TransformHeight(Height, Query.Positions[Index]);

			float& QueryHeight = Query.IndirectHeights[IndirectIndex];

			if (bRelativeHeight &&
				!FVoxelUtilities::IsNaN(Height) &&
				!FVoxelUtilities::IsNaN(QueryHeight))
			{
				Height += QueryHeight;
			}

			QueryHeight = FVoxelHeightHeightChunk::Blend(
				QueryHeight,
				Height,
				BlendMode,
				bApplyOnVoid);
		});
}

void FVoxelHeightSculptInnerData::ApplySurfaceType(
	const FVoxelHeightSparseQuery& Query,
	const FVoxelHeightTransform& StampToQuery,
	const float ScaleXY) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Query.Num());
	ensure(Query.bQuerySurfaceTypes);

	const FVoxelBox2D Bounds = StampToQuery.InverseTransform(Query.PositionBounds).Scale(1. / ScaleXY);

	if (!SurfaceTypeChunkTree->HasChunks(Bounds))
	{
		return;
	}

	const TVoxelHeightChunkTreeIterator<FVoxelHeightSurfaceTypeChunk> Iterator = SurfaceTypeChunkTree->CreateIterator(
		Query,
		StampToQuery,
		ScaleXY);

	ProcessSurfaceTypes(Iterator, [&](
		const int32 Index,
		const float Alpha,
		const FVoxelSurfaceTypeBlend& SurfaceType)
	{
		if (Alpha == 0)
		{
			return;
		}

		const int32 IndirectIndex = Query.GetIndirectIndex(Index);

		FVoxelSurfaceTypeBlend& SurfaceTypeRef = Query.IndirectSurfaceTypes[IndirectIndex];

		FVoxelSurfaceTypeBlend::Lerp(
			SurfaceTypeRef,
			SurfaceTypeRef,
			SurfaceType,
			Alpha);
	});
}

void FVoxelHeightSculptInnerData::ApplyMetadata(
	const FVoxelMetadataRef& MetadataRef,
	const FVoxelHeightSparseQuery& Query,
	const FVoxelHeightTransform& StampToQuery,
	const float ScaleXY) const
{
	const TSharedPtr<TVoxelHeightChunkTree<FVoxelHeightMetadataChunk>> Tree = MetadataRefToChunkTree.FindRef(MetadataRef);
	if (!Tree)
	{
		return;
	}

	VOXEL_FUNCTION_COUNTER_NUM(Query.Num());
	ensure(Query.MetadatasToQuery.Contains(MetadataRef));

	const FVoxelBox2D Bounds = StampToQuery.InverseTransform(Query.PositionBounds).Scale(1. / ScaleXY);
	if (!Tree->HasChunks(Bounds))
	{
		return;
	}

	const TVoxelHeightChunkTreeIterator<FVoxelHeightMetadataChunk> Iterator = Tree->CreateIterator(
		Query,
		StampToQuery,
		ScaleXY);

	if (MetadataRef.GetInnerType().Is<float>())
	{
		FVoxelFloatBuffer& Buffer = Query.IndirectMetadata.FindChecked<FVoxelFloatBuffer>(MetadataRef);

		ProcessMetadatas<float>(Iterator, [&](
			const int32 Index,
			const float Alpha,
			const float Value)
		{
			const int32 IndirectIndex = Query.GetIndirectIndex(Index);

			Buffer.Set(IndirectIndex, FMath::Lerp(Buffer[IndirectIndex], Value, Alpha));
		});
		return;
	}

	if (MetadataRef.GetInnerType().Is<FLinearColor>())
	{
		FVoxelLinearColorBuffer& Buffer = Query.IndirectMetadata.FindChecked<FVoxelLinearColorBuffer>(MetadataRef);

		ProcessMetadatas<FLinearColor>(Iterator, [&](
			const int32 Index,
			const float Alpha,
			const FLinearColor& Value)
		{
			const int32 IndirectIndex = Query.GetIndirectIndex(Index);

			Buffer.Set(IndirectIndex, FMath::Lerp(Buffer[IndirectIndex], Value, Alpha));
		});
		return;
	}

	ensureVoxelSlow(false);
	VOXEL_MESSAGE(Error, "Unsupport metadata type for sculpting: {0}", MetadataRef.GetInnerType().ToString());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelHeightSculptInnerData::GetSurfaceTypes(
	const FVoxelDoubleVector2DBuffer& Positions,
	TVoxelArray<float>& OutAlphas,
	TVoxelArray<FVoxelSurfaceTypeBlend>& OutSurfaceTypes) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Positions.Num());

	FVoxelUtilities::SetNumFast(OutAlphas, Positions.Num());
	FVoxelUtilities::SetNumFast(OutSurfaceTypes, Positions.Num());

	const TVoxelHeightChunkTreeIterator<FVoxelHeightSurfaceTypeChunk> Iterator = SurfaceTypeChunkTree->CreateIterator(Positions);

	ProcessSurfaceTypes(Iterator, [&](
		const int32 Index,
		const float Alpha,
		const FVoxelSurfaceTypeBlend& SurfaceType)
	{
		OutAlphas[Index] = Alpha;
		OutSurfaceTypes[Index] = SurfaceType;
	});
}

void FVoxelHeightSculptInnerData::GetMetadatas(
	const FVoxelMetadataRef& MetadataRef,
	const FVoxelDoubleVector2DBuffer& Positions,
	TVoxelArray<float>& OutAlphas,
	TSharedPtr<FVoxelBuffer>& OutMetadatas) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Positions.Num());

	FVoxelUtilities::SetNumZeroed(OutAlphas, Positions.Num());

	OutMetadatas = MetadataRef.MakeDefaultBuffer(Positions.Num());

	const TSharedPtr<TVoxelHeightChunkTree<FVoxelHeightMetadataChunk>> Tree = MetadataRefToChunkTree.FindRef(MetadataRef);
	if (!Tree)
	{
		return;
	}

	const TVoxelHeightChunkTreeIterator<FVoxelHeightMetadataChunk> Iterator = Tree->CreateIterator(Positions);

	if (MetadataRef.GetInnerType().Is<float>())
	{
		FVoxelFloatBuffer& Buffer = OutMetadatas->AsChecked<FVoxelFloatBuffer>();

		ProcessMetadatas<float>(Iterator, [&](
			const int32 Index,
			const float Alpha,
			const float Value)
		{
			OutAlphas[Index] = Alpha;
			Buffer.Set(Index, Value);
		});
		return;
	}

	if (MetadataRef.GetInnerType().Is<FLinearColor>())
	{
		FVoxelLinearColorBuffer& Buffer = OutMetadatas->AsChecked<FVoxelLinearColorBuffer>();

		ProcessMetadatas<FLinearColor>(Iterator, [&](
			const int32 Index,
			const float Alpha,
			const FLinearColor& Value)
		{
			OutAlphas[Index] = Alpha;
			Buffer.Set(Index, Value);
		});
		return;
	}

	ensureVoxelSlow(false);
	VOXEL_MESSAGE(Error, "Unsupport metadata type for sculpting: {0}", MetadataRef.GetInnerType().ToString());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename LambdaType>
void FVoxelHeightSculptInnerData::ProcessHeights(
	const TVoxelHeightChunkTreeIterator<FVoxelHeightHeightChunk>& Iterator,
	LambdaType Lambda)
{
	Iterator.Iterate(
		[&](const int32 Index,
			const FVoxelHeightHeightChunk* Chunk0, const int32 Index0)
		{
			if (!Chunk0)
			{
				return;
			}

			Lambda(
				Index,
				Chunk0->Heights[Index0]);
		},
		[&](const int32 Index,
			const FVoxelHeightHeightChunk* Chunk0, const int32 Index0,
			const FVoxelHeightHeightChunk* Chunk1, const int32 Index1,
			const float AlphaX)
		{
			if (!Chunk0 ||
				!Chunk1)
			{
				return;
			}

			Lambda(
				Index,
				FMath::Lerp(
					Chunk0->Heights[Index0],
					Chunk1->Heights[Index1],
					AlphaX));
		},
		[&](const int32 Index,
			const FVoxelHeightHeightChunk* Chunk0, const int32 Index0,
			const FVoxelHeightHeightChunk* Chunk1, const int32 Index1,
			const FVoxelHeightHeightChunk* Chunk2, const int32 Index2,
			const FVoxelHeightHeightChunk* Chunk3, const int32 Index3,
			const float AlphaX,
			const float AlphaY)
		{
			if (!Chunk0 ||
				!Chunk1 ||
				!Chunk2 ||
				!Chunk3)
			{
				return;
			}

			Lambda(
				Index,
				FVoxelUtilities::BilinearInterpolation(
					Chunk0->Heights[Index0],
					Chunk1->Heights[Index1],
					Chunk2->Heights[Index2],
					Chunk3->Heights[Index3],
					AlphaX,
					AlphaY));
		});
}

template<typename LambdaType>
void FVoxelHeightSculptInnerData::ProcessSurfaceTypes(
	const TVoxelHeightChunkTreeIterator<FVoxelHeightSurfaceTypeChunk>& Iterator,
	LambdaType Lambda)
{
#define Load(InIndex) \
	if (Chunk ## InIndex) \
	{ \
		Chunk ## InIndex->GetSurfaceType(Index ## InIndex, Alphas[InIndex], SurfaceTypes[InIndex]); \
	} \
	else \
	{ \
		Alphas[InIndex] = 0.f; \
		SurfaceTypes[InIndex].InitializeNull(); \
	}

	Iterator.Iterate(
		[&](const int32 Index,
			const FVoxelHeightSurfaceTypeChunk* Chunk0, const int32 Index0)
		{
			float Alpha0 = 0.f;
			FVoxelSurfaceTypeBlend SurfaceType0;

			if (Chunk0)
			{
				Chunk0->GetSurfaceType(Index0, Alpha0, SurfaceType0);
			}

			Lambda(
				Index,
				Alpha0,
				SurfaceType0);
		},
		[&](const int32 Index,
			const FVoxelHeightSurfaceTypeChunk* Chunk0, const int32 Index0,
			const FVoxelHeightSurfaceTypeChunk* Chunk1, const int32 Index1,
			const float AlphaX)
		{
			TVoxelStaticArray<float, 2> Alphas{ NoInit };
			TVoxelStaticArray<FVoxelSurfaceTypeBlend, 2> SurfaceTypes{ NoInit };

			Load(0);
			Load(1);

			FVoxelSurfaceTypeBlend SurfaceType;
			FVoxelSurfaceTypeBlend::Lerp(
				SurfaceType,
				SurfaceTypes[0],
				SurfaceTypes[1],
				AlphaX);

			Lambda(
				Index,
				FMath::Lerp(
					Alphas[0],
					Alphas[1],
					AlphaX),
				SurfaceType);
		},
		[&](const int32 Index,
			const FVoxelHeightSurfaceTypeChunk* Chunk0, const int32 Index0,
			const FVoxelHeightSurfaceTypeChunk* Chunk1, const int32 Index1,
			const FVoxelHeightSurfaceTypeChunk* Chunk2, const int32 Index2,
			const FVoxelHeightSurfaceTypeChunk* Chunk3, const int32 Index3,
			const float AlphaX,
			const float AlphaY)
		{
			TVoxelStaticArray<float, 4> Alphas{ NoInit };
			TVoxelStaticArray<FVoxelSurfaceTypeBlend, 4> SurfaceTypes{ NoInit };

			Load(0);
			Load(1);
			Load(2);
			Load(3);

			FVoxelSurfaceTypeBlend SurfaceType;
			FVoxelSurfaceTypeBlend::BilinearInterpolation(
				SurfaceType,
				SurfaceTypes,
				AlphaX,
				AlphaY);

			Lambda(
				Index,
				FVoxelUtilities::BilinearInterpolation(
					Alphas[0],
					Alphas[1],
					Alphas[2],
					Alphas[3],
					AlphaX,
					AlphaY),
				SurfaceType);
		});

#undef Load
}

template<typename InnerType, typename LambdaType>
void FVoxelHeightSculptInnerData::ProcessMetadatas(
	const TVoxelHeightChunkTreeIterator<FVoxelHeightMetadataChunk>& Iterator,
	LambdaType Lambda)
{
	Iterator.Iterate(
		[&](const int32 Index,
			const FVoxelHeightMetadataChunk* Chunk0, const int32 Index0)
		{
			if (!Chunk0)
			{
				return;
			}

			Lambda(
				Index,
				Chunk0->GetAlpha(Index0),
				Chunk0->GetValue<InnerType>(Index0));
		},
		[&](const int32 Index,
			const FVoxelHeightMetadataChunk* Chunk0, const int32 Index0,
			const FVoxelHeightMetadataChunk* Chunk1, const int32 Index1,
			const float AlphaX)
		{
			if (!Chunk0 ||
				!Chunk1)
			{
				return;
			}

			Lambda(
				Index,
				FMath::Lerp(
					Chunk0->GetAlpha(Index0),
					Chunk1->GetAlpha(Index1),
					AlphaX),
				FMath::Lerp(
					Chunk0->GetValue<InnerType>(Index0),
					Chunk1->GetValue<InnerType>(Index1),
					AlphaX));
		},
		[&](const int32 Index,
			const FVoxelHeightMetadataChunk* Chunk0, const int32 Index0,
			const FVoxelHeightMetadataChunk* Chunk1, const int32 Index1,
			const FVoxelHeightMetadataChunk* Chunk2, const int32 Index2,
			const FVoxelHeightMetadataChunk* Chunk3, const int32 Index3,
			const float AlphaX,
			const float AlphaY)
		{
			if (!Chunk0 ||
				!Chunk1 ||
				!Chunk2 ||
				!Chunk3)
			{
				return;
			}

			Lambda(
				Index,
				FVoxelUtilities::BilinearInterpolation(
					Chunk0->GetAlpha(Index0),
					Chunk1->GetAlpha(Index1),
					Chunk2->GetAlpha(Index2),
					Chunk3->GetAlpha(Index3),
					AlphaX,
					AlphaY),
				FVoxelUtilities::BilinearInterpolation(
					Chunk0->GetValue<InnerType>(Index0),
					Chunk1->GetValue<InnerType>(Index1),
					Chunk2->GetValue<InnerType>(Index2),
					Chunk3->GetValue<InnerType>(Index3),
					AlphaX,
					AlphaY));
		});
}