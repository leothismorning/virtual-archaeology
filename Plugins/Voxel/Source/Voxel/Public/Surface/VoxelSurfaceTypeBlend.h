// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelSurfaceType.h"
#include "VoxelSurfaceTypeBlend.generated.h"

struct alignas(8) VOXEL_API FVoxelSurfaceTypeBlendLayer
{
	FVoxelSurfaceType Type;
	uint16 Padding = 0;
	float Weight = 0;

	FVoxelSurfaceTypeBlendLayer() = default;

	FORCEINLINE FVoxelSurfaceTypeBlendLayer(
		const FVoxelSurfaceType& Type,
		const float Weight)
		: Type(Type)
		, Weight(Weight)
	{
	}

	FORCEINLINE bool operator==(const FVoxelSurfaceTypeBlendLayer& Other) const
	{
		return ReinterpretCastRef<uint64>(*this) == ReinterpretCastRef<uint64>(Other);
	}

	FString GetSurfaceName() const;
	FString GetWeightString() const;
};

struct alignas(8) VOXEL_API FVoxelSurfaceTypeBlendBase
{
	static constexpr int32 MaxLayers = 15;

	TVoxelStaticArray<FVoxelSurfaceTypeBlendLayer, MaxLayers> Layers{ NoInit };
	int32 NumLayers = 0;

	void SortByType();
	void SortByWeight();
};

USTRUCT()
struct alignas(8) VOXEL_API FVoxelSurfaceTypeBlend
#if CPP
	: private FVoxelSurfaceTypeBlendBase
#endif
{
	GENERATED_BODY()

public:
	using FVoxelSurfaceTypeBlendBase::MaxLayers;

public:
	FORCEINLINE FVoxelSurfaceTypeBlend()
	{
		NumLayers = 0;
	}
	FORCEINLINE FVoxelSurfaceTypeBlend(const FVoxelSurfaceTypeBlend& Other)
	{
		*this = Other;
	}

	// *this = *this is safe as we have no allocations
	FORCEINLINE FVoxelSurfaceTypeBlend& operator=(const FVoxelSurfaceTypeBlend& Other)
	{
		NumLayers = Other.NumLayers;

		for (int32 Index = 0; Index < NumLayers; Index++)
		{
			Layers[Index] = Other.Layers[Index];
		}

		return *this;
	}

	FORCEINLINE static FVoxelSurfaceTypeBlend FromType(const FVoxelSurfaceType Type)
	{
		FVoxelSurfaceTypeBlend Result;
		Result.InitializeFromType(Type);
		return Result;
	}

	FORCEINLINE void InitializeNull()
	{
		NumLayers = 0;
	}
	FORCEINLINE void InitializeFromType(const FVoxelSurfaceType Type)
	{
		if (Type.IsNull())
		{
			NumLayers = 0;
			return;
		}

		Layers[0] = FVoxelSurfaceTypeBlendLayer(Type, 1.f);
		NumLayers = 1;
	}

	// NewLayers must be unique indices with positive weight
	// It doesn't need to be sorted
	void InitializeFromLayers(TVoxelArrayView<FVoxelSurfaceTypeBlendLayer> NewLayers);

public:
	FORCEINLINE bool IsNull() const
	{
		Check();
		return NumLayers == 0;
	}

	FORCEINLINE const FVoxelSurfaceTypeBlendBase& AsBase() const
	{
		return *this;
	}

	FORCEINLINE TConstVoxelArrayView<FVoxelSurfaceTypeBlendLayer> GetLayers() const
	{
		Check();
		return Layers.View().LeftOf(NumLayers);
	}

	FORCEINLINE bool operator==(const FVoxelSurfaceTypeBlend& Other) const
	{
		Check();
		Other.Check();

		if (NumLayers != Other.NumLayers)
		{
			return false;
		}

		if (NumLayers == 1)
		{
			// Ignore the weight, we might be slightly off due to float imprecision
			return Layers[0].Type == Other.Layers[0].Type;
		}

		for (int32 Index = 0; Index < NumLayers; Index++)
		{
			if (Layers[Index] != Other.Layers[Index])
			{
				return false;
			}
		}

		return true;
	}

	FVoxelSurfaceTypeBlendLayer GetTopLayer() const;
	TVoxelArray<FVoxelSurfaceTypeBlendLayer> GetLayersSortedByWeight() const;

	bool Equals(
		const FVoxelSurfaceTypeBlend& Other,
		float Tolerance = KINDA_SMALL_NUMBER) const;

	void PopLayersForRendering();
	void Add(FVoxelSurfaceTypeBlendLayer Layer);
	void Remove(FVoxelSurfaceType Type);

public:
	static void Lerp(
		FVoxelSurfaceTypeBlend& OutResult,
		const FVoxelSurfaceTypeBlend& BlendA,
		const FVoxelSurfaceTypeBlend& BlendB,
		float Alpha);

	static void Lerp(
		FVoxelSurfaceTypeBlend& OutResult,
		const FVoxelSurfaceTypeBlend& BlendA,
		const FVoxelSurfaceType& SurfaceB,
		float Alpha);

public:
	static void BilinearInterpolation(
		FVoxelSurfaceTypeBlend& OutResult,
		TConstVoxelArrayView<FVoxelSurfaceTypeBlend> Blends,
		float AlphaX,
		float AlphaY);

	static void TrilinearInterpolation(
		FVoxelSurfaceTypeBlend& OutResult,
		TConstVoxelArrayView<FVoxelSurfaceTypeBlend> Blends,
		float AlphaX,
		float AlphaY,
		float AlphaZ);

private:
	void CheckImpl() const;
	void PopLowestLayer();

	static void PopToMaxLayer_DoNotNormalize(TVoxelArrayView<FVoxelSurfaceTypeBlendLayer> Layers);
	static void PopToMaxLayer_Normalize(TVoxelArrayView<FVoxelSurfaceTypeBlendLayer> Layers);

private:
	FORCEINLINE void Check() const
	{
#if VOXEL_DEBUG
		CheckImpl();
#endif
	}
};
checkStatic(sizeof(FVoxelSurfaceTypeBlend) == 128);
checkStatic(std::is_trivially_destructible_v<FVoxelSurfaceTypeBlend>);