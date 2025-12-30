// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Surface/VoxelSurfaceTypeBlend.h"
#include "VoxelSortNetwork.h"

FString FVoxelSurfaceTypeBlendLayer::GetSurfaceName() const
{
	return Type.GetName();
}

FString FVoxelSurfaceTypeBlendLayer::GetWeightString() const
{
	const float Percent = Weight * 100.f;

	int32 NumFractionalDigits = 0;
	if (Percent < 1.f)
	{
		NumFractionalDigits = 2;
	}
	else if (Percent < 10.f)
	{
		NumFractionalDigits = 1;
	}

	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = NumFractionalDigits;
	Options.MaximumFractionalDigits = NumFractionalDigits;

	return FText::AsNumber(Percent, &Options).ToString() + "%";
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelSurfaceTypeBlendBase::SortByType()
{
	const auto Swap = [&](const int32 A, const int32 B)
	{
		if (Layers[A].Type > Layers[B].Type)
		{
			::Swap(Layers[A], Layers[B]);
		}
	};

	switch (NumLayers)
	{
	default: VOXEL_ASSUME(false);
	case 0: return;
	case 1: return;
	case 2: TVoxelSortNetwork<2>::Apply(Swap); return;
	case 3: TVoxelSortNetwork<3>::Apply(Swap); return;
	case 4: TVoxelSortNetwork<4>::Apply(Swap); return;
	case 5: TVoxelSortNetwork<5>::Apply(Swap); return;
	case 6: TVoxelSortNetwork<6>::Apply(Swap); return;
	case 7: TVoxelSortNetwork<7>::Apply(Swap); return;
	case 8: TVoxelSortNetwork<8>::Apply(Swap); return;
	case 9: TVoxelSortNetwork<9>::Apply(Swap); return;
	case 10: TVoxelSortNetwork<10>::Apply(Swap); return;
	case 11: TVoxelSortNetwork<11>::Apply(Swap); return;
	case 12: TVoxelSortNetwork<12>::Apply(Swap); return;
	case 13: TVoxelSortNetwork<13>::Apply(Swap); return;
	case 14: TVoxelSortNetwork<14>::Apply(Swap); return;
	case 15: TVoxelSortNetwork<15>::Apply(Swap); return;
	}
}

void FVoxelSurfaceTypeBlendBase::SortByWeight()
{
	const auto Swap = [&](const int32 A, const int32 B)
	{
		if (Layers[A].Weight < Layers[B].Weight)
		{
			::Swap(Layers[A], Layers[B]);
		}
	};

	switch (NumLayers)
	{
	default: VOXEL_ASSUME(false);
	case 0: return;
	case 1: return;
	case 2: TVoxelSortNetwork<2>::Apply(Swap); return;
	case 3: TVoxelSortNetwork<3>::Apply(Swap); return;
	case 4: TVoxelSortNetwork<4>::Apply(Swap); return;
	case 5: TVoxelSortNetwork<5>::Apply(Swap); return;
	case 6: TVoxelSortNetwork<6>::Apply(Swap); return;
	case 7: TVoxelSortNetwork<7>::Apply(Swap); return;
	case 8: TVoxelSortNetwork<8>::Apply(Swap); return;
	case 9: TVoxelSortNetwork<9>::Apply(Swap); return;
	case 10: TVoxelSortNetwork<10>::Apply(Swap); return;
	case 11: TVoxelSortNetwork<11>::Apply(Swap); return;
	case 12: TVoxelSortNetwork<12>::Apply(Swap); return;
	case 13: TVoxelSortNetwork<13>::Apply(Swap); return;
	case 14: TVoxelSortNetwork<14>::Apply(Swap); return;
	case 15: TVoxelSortNetwork<15>::Apply(Swap); return;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelSurfaceTypeBlend::InitializeFromLayers(TVoxelArrayView<FVoxelSurfaceTypeBlendLayer> NewLayers)
{
#if VOXEL_DEBUG
	TVoxelInlineArray<FVoxelSurfaceType, 16> Indices;
	for (const FVoxelSurfaceTypeBlendLayer& Layer : NewLayers)
	{
		ensure(!Indices.Contains(Layer.Type));
		Indices.Add(Layer.Type);

		ensure(FVoxelUtilities::IsFinite(Layer.Weight));
		ensure(0.f < Layer.Weight);
	}
#endif

	if (NewLayers.Num() == 0)
	{
		NumLayers = 0;
		return;
	}

	if (NewLayers.Num() > MaxLayers)
	{
		PopToMaxLayer_DoNotNormalize(NewLayers);
		NewLayers = NewLayers.LeftOf(MaxLayers);
	}

	float Multiplier;
	{
		float WeightSum = 0;
		for (const FVoxelSurfaceTypeBlendLayer& Layer : NewLayers)
		{
			WeightSum += Layer.Weight;
		}
		checkVoxelSlow(WeightSum > 0);

		Multiplier = 1.f / WeightSum;
	}

	NumLayers = NewLayers.Num();

	for (int32 Index = 0; Index < NewLayers.Num(); Index++)
	{
		FVoxelSurfaceTypeBlendLayer Layer = NewLayers[Index];
		Layer.Weight *= Multiplier;
		Layers[Index] = Layer;
	}

	SortByType();
	Check();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelSurfaceTypeBlendLayer FVoxelSurfaceTypeBlend::GetTopLayer() const
{
	Check();

	if (NumLayers == 0)
	{
		return {};
	}

	FVoxelSurfaceTypeBlendLayer Result = Layers[0];
	for (int32 Index = 1; Index < NumLayers; Index++)
	{
		const FVoxelSurfaceTypeBlendLayer& Layer = Layers[Index];

		if (Layer.Weight > Result.Weight)
		{
			Result = Layer;
		}
	}
	return Result;
}

TVoxelArray<FVoxelSurfaceTypeBlendLayer> FVoxelSurfaceTypeBlend::GetLayersSortedByWeight() const
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelArray<FVoxelSurfaceTypeBlendLayer> SortedLayers = GetLayers().Array();
	SortedLayers.Sort([](const FVoxelSurfaceTypeBlendLayer& A, const FVoxelSurfaceTypeBlendLayer& B)
	{
		return A.Weight > B.Weight;
	});
	return SortedLayers;
}

bool FVoxelSurfaceTypeBlend::Equals(
	const FVoxelSurfaceTypeBlend& Other,
	const float Tolerance) const
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
		const FVoxelSurfaceTypeBlendLayer& Layer = Layers[0];
		const FVoxelSurfaceTypeBlendLayer& OtherLayer = Other.Layers[0];

		if (Layer.Type != OtherLayer.Type ||
			!FMath::IsNearlyEqual(Layer.Weight, OtherLayer.Weight, Tolerance))
		{
			return false;
		}
	}

	return true;
}

void FVoxelSurfaceTypeBlend::PopLayersForRendering()
{
	VOXEL_FUNCTION_COUNTER();
	checkVoxelSlow(NumLayers > 8);

	while (NumLayers > 8)
	{
		float LowestWeight = Layers[0].Weight;
		int32 LowestIndex = 0;

		for (int32 Index = 1; Index < NumLayers; Index++)
		{
			const FVoxelSurfaceTypeBlendLayer& Layer = Layers[Index];
			if (Layer.Weight > LowestWeight)
			{
				continue;
			}

			LowestWeight = Layer.Weight;
			LowestIndex = Index;
		}

		checkVoxelSlow(LowestWeight > 0);

		// Move data
		for (int32 Index = LowestIndex + 1; Index < NumLayers; Index++)
		{
			Layers[Index - 1] = Layers[Index];
		}

		NumLayers--;
	}

	checkVoxelSlow(NumLayers == 8);
}

void FVoxelSurfaceTypeBlend::Add(const FVoxelSurfaceTypeBlendLayer Layer)
{
	checkVoxelSlow(Layer.Weight >= 0);

	Check();
	ON_SCOPE_EXIT
	{
		// Re-normalize
		const float Multiplier = 1. / (1.f + Layer.Weight);

		for (int32 Index = 0; Index < NumLayers; Index++)
		{
			Layers[Index].Weight *= Multiplier;
		}

		Check();
	};

	if (Layer.Type.IsNull() ||
		Layer.Weight == 0)
	{
		return;
	}

	for (int32 Index = 0; Index < NumLayers; Index++)
	{
		const FVoxelSurfaceTypeBlendLayer& OtherLayer = Layers[Index];

		if (OtherLayer.Type == Layer.Type)
		{
			Layers[Index].Weight += Layer.Weight;
			return;
		}

		if (OtherLayer.Type > Layer.Type)
		{
			break;
		}
	}

	if (NumLayers == MaxLayers)
	{
		ensureVoxelSlow(false);
		PopLowestLayer();
	}
	checkVoxelSlow(NumLayers < MaxLayers);

	for (int32 Index = 0; Index < NumLayers; Index++)
	{
		const FVoxelSurfaceTypeBlendLayer& OtherLayer = Layers[Index];
		checkVoxelSlow(Layer.Type != OtherLayer.Type);

		if (OtherLayer.Type < Layer.Type)
		{
			continue;
		}

		// Move data

		for (int32 OtherIndex = NumLayers; OtherIndex >= Index; OtherIndex--)
		{
			Layers[OtherIndex + 1] = Layers[OtherIndex];
		}
		NumLayers++;

		Layers[Index] = Layer;
		return;
	}

	Layers[NumLayers++] = Layer;
}

void FVoxelSurfaceTypeBlend::Remove(const FVoxelSurfaceType Type)
{
	ON_SCOPE_EXIT
	{
		Check();
	};

	for (int32 Index = 0; Index < NumLayers; Index++)
	{
		const FVoxelSurfaceTypeBlendLayer Layer = Layers[Index];
		if (Layer.Type < Type)
		{
			continue;
		}
		if (Layer.Type > Type)
		{
			return;
		}
		checkVoxelSlow(Layer.Type == Type);

		// Move data

		for (int32 OtherIndex = Index + 1; OtherIndex < NumLayers; OtherIndex++)
		{
			Layers[OtherIndex - 1] = Layers[OtherIndex];
		}
		NumLayers--;

		// Re-normalize

		double Sum = 0.;
		for (int32 OtherIndex = 0; OtherIndex < NumLayers; OtherIndex++)
		{
			Sum += Layers[OtherIndex].Weight;
		}

		for (int32 OtherIndex = 0; OtherIndex < NumLayers; OtherIndex++)
		{
			Layers[OtherIndex].Weight /= Sum;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelSurfaceTypeBlend::Lerp(
	FVoxelSurfaceTypeBlend& OutResult,
	const FVoxelSurfaceTypeBlend& BlendA,
	const FVoxelSurfaceTypeBlend& BlendB,
	const float Alpha)
{
	BlendA.Check();
	BlendB.Check();
	ensureVoxelSlow(0 <= Alpha && Alpha <= 1);

	ON_SCOPE_EXIT
	{
		OutResult.Check();
	};

	if (Alpha == 0)
	{
		OutResult = BlendA;
		return;
	}
	if (Alpha == 1)
	{
		OutResult = BlendB;
		return;
	}

	if (BlendA.IsNull())
	{
		OutResult = BlendB;
		return;
	}
	if (BlendB.IsNull())
	{
		OutResult = BlendA;
		return;
	}

	TVoxelStaticArray<FVoxelSurfaceTypeBlendLayer, 2 * MaxLayers> Layers{ NoInit };
	int32 NumLayers = 0;

	const float AlphaA = 1.f - Alpha;
	const float AlphaB = Alpha;

	int32 IndexA = 0;
	int32 IndexB = 0;
	while (
		IndexA < BlendA.NumLayers &&
		IndexB < BlendB.NumLayers)
	{
		const FVoxelSurfaceTypeBlendLayer& LayerA = BlendA.Layers[IndexA];
		const FVoxelSurfaceTypeBlendLayer& LayerB = BlendB.Layers[IndexB];

		if (LayerA.Type < LayerB.Type)
		{
			Layers[NumLayers++] = FVoxelSurfaceTypeBlendLayer(LayerA.Type, LayerA.Weight * AlphaA);
			IndexA++;
		}
		else if (LayerB.Type < LayerA.Type)
		{
			Layers[NumLayers++] = FVoxelSurfaceTypeBlendLayer(LayerB.Type, LayerB.Weight * AlphaB);
			IndexB++;
		}
		else
		{
			checkVoxelSlow(LayerA.Type == LayerB.Type);

			Layers[NumLayers++] = FVoxelSurfaceTypeBlendLayer(
				LayerA.Type,
				LayerA.Weight * AlphaA + LayerB.Weight * AlphaB);

			IndexA++;
			IndexB++;
		}
	}

	while (IndexA < BlendA.NumLayers)
	{
		const FVoxelSurfaceTypeBlendLayer& LayerA = BlendA.Layers[IndexA];
		Layers[NumLayers++] = FVoxelSurfaceTypeBlendLayer(LayerA.Type, LayerA.Weight * AlphaA);
		IndexA++;
	}

	while (IndexB < BlendB.NumLayers)
	{
		const FVoxelSurfaceTypeBlendLayer& LayerB = BlendB.Layers[IndexB];
		Layers[NumLayers++] = FVoxelSurfaceTypeBlendLayer(LayerB.Type, LayerB.Weight * AlphaB);
		IndexB++;
	}

	if (NumLayers > MaxLayers)
	{
		ensureVoxelSlow(false);
		PopToMaxLayer_Normalize(Layers.View().LeftOf(NumLayers));
		NumLayers = MaxLayers;
	}

	OutResult.NumLayers = NumLayers;

	for (int32 Index = 0; Index < NumLayers; Index++)
	{
		OutResult.Layers[Index] = Layers[Index];
	}
}

void FVoxelSurfaceTypeBlend::Lerp(
	FVoxelSurfaceTypeBlend& OutResult,
	const FVoxelSurfaceTypeBlend& BlendA,
	const FVoxelSurfaceType& SurfaceB,
	const float Alpha)
{
	BlendA.Check();
	ensureVoxelSlow(0 <= Alpha && Alpha <= 1);

#if VOXEL_DEBUG
	const FVoxelSurfaceTypeBlend BlendACopy = BlendA;
#endif

	ON_SCOPE_EXIT
	{
		OutResult.Check();

#if VOXEL_DEBUG
		FVoxelSurfaceTypeBlend ExpectedResult;
		Lerp(ExpectedResult, BlendACopy, FromType(SurfaceB), Alpha);
		ensure(OutResult.Equals(ExpectedResult, 0.01f));
#endif
	};

	if (Alpha == 0)
	{
		OutResult = BlendA;
		return;
	}
	if (Alpha == 1)
	{
		OutResult.InitializeFromType(SurfaceB);
		return;
	}

	if (BlendA.IsNull())
	{
		OutResult.InitializeFromType(SurfaceB);
		return;
	}
	if (SurfaceB.IsNull())
	{
		OutResult = BlendA;
		return;
	}

	TVoxelStaticArray<FVoxelSurfaceTypeBlendLayer, MaxLayers + 1> Layers{ NoInit };
	int32 NumLayers = 0;

	const float AlphaA = 1.f - Alpha;
	const float AlphaB = Alpha;

	bool bAdded = false;
	for (int32 Index = 0; Index < BlendA.NumLayers; Index++)
	{
		FVoxelSurfaceTypeBlendLayer Layer = BlendA.Layers[Index];
		Layer.Weight *= AlphaA;

		if (SurfaceB < Layer.Type &&
			!bAdded)
		{
			bAdded = true;
			Layers[NumLayers++] = FVoxelSurfaceTypeBlendLayer(SurfaceB, AlphaB);
		}

		if (Layer.Type == SurfaceB)
		{
			checkVoxelSlow(!bAdded);
			bAdded = true;

			Layer.Weight += AlphaB;
		}

		Layers[NumLayers++] = Layer;
	}

	if (!bAdded)
	{
		Layers[NumLayers++] = FVoxelSurfaceTypeBlendLayer(SurfaceB, AlphaB);
	}

	if (NumLayers > MaxLayers)
	{
		ensureVoxelSlow(false);
		PopToMaxLayer_Normalize(Layers.View().LeftOf(NumLayers));
		NumLayers = MaxLayers;
	}

	OutResult.NumLayers = NumLayers;

	for (int32 Index = 0; Index < NumLayers; Index++)
	{
		OutResult.Layers[Index] = Layers[Index];
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelSurfaceTypeBlend::BilinearInterpolation(
	FVoxelSurfaceTypeBlend& OutResult,
	const TConstVoxelArrayView<FVoxelSurfaceTypeBlend> Blends,
	const float AlphaX,
	const float AlphaY)
{
	checkVoxelSlow(Blends.Num() == 4);

	for (const FVoxelSurfaceTypeBlend& Blend : Blends)
	{
		Blend.Check();
	}

	ensureVoxelSlow(0 <= AlphaX && AlphaX <= 1);
	ensureVoxelSlow(0 <= AlphaY && AlphaY <= 1);

	ON_SCOPE_EXIT
	{
		OutResult.Check();

#if VOXEL_DEBUG && 0
		FVoxelSurfaceTypeBlend AB;
		FVoxelSurfaceTypeBlend CD;
		Lerp(AB, Blends[0], Blends[1], AlphaX);
		Lerp(CD, Blends[1], Blends[2], AlphaX);

		FVoxelSurfaceTypeBlend ExpectedResult;
		Lerp(ExpectedResult, AB, CD, AlphaY);
		ensure(OutResult.Equals(ExpectedResult, 0.01f));
#endif
	};

	if (AlphaX == 0)
	{
		Lerp(OutResult, Blends[0], Blends[2], AlphaY);
		return;
	}
	if (AlphaX == 1)
	{
		Lerp(OutResult, Blends[1], Blends[3], AlphaY);
		return;
	}

	if (AlphaY == 0)
	{
		Lerp(OutResult, Blends[0], Blends[1], AlphaX);
		return;
	}
	if (AlphaY == 1)
	{
		Lerp(OutResult, Blends[2], Blends[3], AlphaX);
		return;
	}

	TVoxelStaticArray<double, 4> Alphas{ NoInit };
	Alphas[0] = double(1.f - AlphaY) * double(1.f - AlphaX);
	Alphas[1] = double(1.f - AlphaY) * double(AlphaX);
	Alphas[2] = double(AlphaY) * double(1.f - AlphaX);
	Alphas[3] = double(AlphaY) * double(AlphaX);

	TVoxelStaticArray<FVoxelSurfaceTypeBlendLayer, 4 * MaxLayers> Layers{ NoInit };
	int32 NumLayers = 0;

	TVoxelStaticArray<int32, 4> LayerIndices{ ForceInit };
	while (true)
	{
		FVoxelSurfaceType BestLayerType = ReinterpretCastRef<FVoxelSurfaceType>(MAX_uint16);
		double BestLayerWeight = 0.;

		for (int32 Index = 0; Index < 4; Index++)
		{
			const FVoxelSurfaceTypeBlend& Blend = Blends[Index];
			const int32 LayerIndex = LayerIndices[Index];

			if (LayerIndex >= Blend.NumLayers)
			{
				continue;
			}

			const FVoxelSurfaceTypeBlendLayer Layer = Blend.Layers[LayerIndex];

			if (Layer.Type < BestLayerType)
			{
				BestLayerType = Layer.Type;
				BestLayerWeight = Layer.Weight * Alphas[Index];
			}
			else if (Layer.Type == BestLayerType)
			{
				BestLayerWeight += Layer.Weight * Alphas[Index];
			}
		}

		if (ReinterpretCastRef<uint16>(BestLayerType) == MAX_uint16)
		{
			break;
		}

		for (int32 Index = 0; Index < 4; Index++)
		{
			const FVoxelSurfaceTypeBlend& Blend = Blends[Index];
			int32& LayerIndex = LayerIndices[Index];

			if (LayerIndex >= Blend.NumLayers)
			{
				continue;
			}

			if (Blend.Layers[LayerIndex].Type == BestLayerType)
			{
				LayerIndex++;
			}
		}

		Layers[NumLayers++] = FVoxelSurfaceTypeBlendLayer(BestLayerType, BestLayerWeight);
	}

	if (NumLayers > MaxLayers)
	{
		ensureVoxelSlow(false);
		PopToMaxLayer_Normalize(Layers.View().LeftOf(NumLayers));
		NumLayers = MaxLayers;
	}

	// Re-normalize in case there's a null in Blends
	double Sum = 0.;
	for (int32 Index = 0; Index < NumLayers; Index++)
	{
		Sum += Layers[Index].Weight;
	}

	OutResult.NumLayers = NumLayers;

	for (int32 Index = 0; Index < NumLayers; Index++)
	{
		FVoxelSurfaceTypeBlendLayer Layer = Layers[Index];
		Layer.Weight /= Sum;
		OutResult.Layers[Index] = Layer;
	}
}

void FVoxelSurfaceTypeBlend::TrilinearInterpolation(
	FVoxelSurfaceTypeBlend& OutResult,
	const TConstVoxelArrayView<FVoxelSurfaceTypeBlend> Blends,
	const float AlphaX,
	const float AlphaY,
	const float AlphaZ)
{
	checkVoxelSlow(Blends.Num() == 8);

	for (const FVoxelSurfaceTypeBlend& Blend : Blends)
	{
		Blend.Check();
	}

	ensureVoxelSlow(0 <= AlphaX && AlphaX <= 1);
	ensureVoxelSlow(0 <= AlphaY && AlphaY <= 1);
	ensureVoxelSlow(0 <= AlphaZ && AlphaZ <= 1);

	ON_SCOPE_EXIT
	{
		OutResult.Check();

#if VOXEL_DEBUG && 0
		FVoxelSurfaceTypeBlend ABCD;
		FVoxelSurfaceTypeBlend EFGH;
		BilinearInterpolation(ABCD, Blends.LeftOf(4), AlphaX, AlphaY);
		BilinearInterpolation(EFGH, Blends.RightOf(4), AlphaX, AlphaY);

		FVoxelSurfaceTypeBlend ExpectedResult;
		Lerp(ExpectedResult, ABCD, EFGH, AlphaZ);
		ensure(OutResult.Equals(ExpectedResult, 0.01f));
#endif
	};

	if (AlphaZ == 0)
	{
		BilinearInterpolation(OutResult, Blends.LeftOf(4), AlphaX, AlphaY);
		return;
	}
	if (AlphaZ == 1)
	{
		BilinearInterpolation(OutResult, Blends.RightOf(4), AlphaX, AlphaY);
		return;
	}

	TVoxelStaticArray<double, 8> Alphas{ NoInit };
	Alphas[0] = double(1.f - AlphaZ) * double(1.f - AlphaY) * double(1.f - AlphaX);
	Alphas[1] = double(1.f - AlphaZ) * double(1.f - AlphaY) * double(AlphaX);
	Alphas[2] = double(1.f - AlphaZ) * double(AlphaY) * double(1.f - AlphaX);
	Alphas[3] = double(1.f - AlphaZ) * double(AlphaY) * double(AlphaX);
	Alphas[4] = double(AlphaZ) * double(1.f - AlphaY) * double(1.f - AlphaX);
	Alphas[5] = double(AlphaZ) * double(1.f - AlphaY) * double(AlphaX);
	Alphas[6] = double(AlphaZ) * double(AlphaY) * double(1.f - AlphaX);
	Alphas[7] = double(AlphaZ) * double(AlphaY) * double(AlphaX);

	TVoxelStaticArray<FVoxelSurfaceTypeBlendLayer, 8 * MaxLayers> Layers{ NoInit };
	int32 NumLayers = 0;

	TVoxelStaticArray<int32, 8> LayerIndices{ ForceInit };
	while (true)
	{
		FVoxelSurfaceType BestLayerType = ReinterpretCastRef<FVoxelSurfaceType>(MAX_uint16);
		double BestLayerWeight = 0.;

		for (int32 Index = 0; Index < 8; Index++)
		{
			const FVoxelSurfaceTypeBlend& Blend = Blends[Index];
			const int32 LayerIndex = LayerIndices[Index];

			if (LayerIndex >= Blend.NumLayers)
			{
				continue;
			}

			const FVoxelSurfaceTypeBlendLayer Layer = Blend.Layers[LayerIndex];

			if (Layer.Type < BestLayerType)
			{
				BestLayerType = Layer.Type;
				BestLayerWeight = Layer.Weight * Alphas[Index];
			}
			else if (Layer.Type == BestLayerType)
			{
				BestLayerWeight += Layer.Weight * Alphas[Index];
			}
		}

		if (ReinterpretCastRef<uint16>(BestLayerType) == MAX_uint16)
		{
			break;
		}

		for (int32 Index = 0; Index < 8; Index++)
		{
			const FVoxelSurfaceTypeBlend& Blend = Blends[Index];
			int32& LayerIndex = LayerIndices[Index];

			if (LayerIndex >= Blend.NumLayers)
			{
				continue;
			}

			if (Blend.Layers[LayerIndex].Type == BestLayerType)
			{
				LayerIndex++;
			}
		}

		Layers[NumLayers++] = FVoxelSurfaceTypeBlendLayer(BestLayerType, BestLayerWeight);
	}

	if (NumLayers > MaxLayers)
	{
		ensureVoxelSlow(false);
		PopToMaxLayer_Normalize(Layers.View().LeftOf(NumLayers));
		NumLayers = MaxLayers;
	}

	// Re-normalize in case there's a null in Blends
	double Sum = 0.;
	for (int32 Index = 0; Index < NumLayers; Index++)
	{
		Sum += Layers[Index].Weight;
	}

	OutResult.NumLayers = NumLayers;

	for (int32 Index = 0; Index < NumLayers; Index++)
	{
		FVoxelSurfaceTypeBlendLayer Layer = Layers[Index];
		Layer.Weight /= Sum;
		OutResult.Layers[Index] = Layer;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelSurfaceTypeBlend::CheckImpl() const
{
	check(0 <= NumLayers && NumLayers <= MaxLayers);

	if (NumLayers == 0)
	{
		return;
	}

	float WeightSum = 0.f;
	for (int32 Index = 0; Index < NumLayers; Index++)
	{
		const FVoxelSurfaceTypeBlendLayer& Layer = Layers[Index];
		ensure(-KINDA_SMALL_NUMBER <= Layer.Weight && Layer.Weight <= 1.f + KINDA_SMALL_NUMBER);
		ensure(FVoxelUtilities::IsFinite(Layer.Weight));
		ensure(!Layer.Type.IsNull());
		ensure(Layer.Weight > 0);

		WeightSum += Layer.Weight;

		if (Index != 0)
		{
			ensure(Layers[Index - 1].Type < Layer.Type);
		}
	}
	ensure(FMath::IsNearlyEqual(WeightSum, 1.f, 0.001f));
}

void FVoxelSurfaceTypeBlend::PopLowestLayer()
{
	checkVoxelSlow(NumLayers == MaxLayers);

	Check();
	ON_SCOPE_EXIT
	{
		Check();
	};

	float LowestWeight = Layers[0].Weight;
	int32 LowestIndex = 0;

	for (int32 Index = 1; Index < NumLayers; Index++)
	{
		const FVoxelSurfaceTypeBlendLayer& Layer = Layers[Index];
		if (Layer.Weight > LowestWeight)
		{
			continue;
		}

		LowestWeight = Layer.Weight;
		LowestIndex = Index;
	}

	checkVoxelSlow(LowestWeight > 0);

	// Move data
	for (int32 Index = LowestIndex + 1; Index < NumLayers; Index++)
	{
		Layers[Index - 1] = Layers[Index];
	}

	NumLayers--;

	// Re-normalize
	const float Multiplier = 1. / (1.f - LowestWeight);

	for (int32 Index = 0; Index < NumLayers; Index++)
	{
		Layers[Index].Weight *= Multiplier;
	}
}

void FVoxelSurfaceTypeBlend::PopToMaxLayer_DoNotNormalize(TVoxelArrayView<FVoxelSurfaceTypeBlendLayer> Layers)
{
	VOXEL_FUNCTION_COUNTER();
	checkVoxelSlow(Layers.Num() > MaxLayers);

	while (Layers.Num() > MaxLayers)
	{
		float LowestWeight = Layers[0].Weight;
		int32 LowestIndex = 0;

		for (int32 Index = 1; Index < Layers.Num(); Index++)
		{
			const FVoxelSurfaceTypeBlendLayer& Layer = Layers[Index];
			if (Layer.Weight > LowestWeight)
			{
				continue;
			}

			LowestWeight = Layer.Weight;
			LowestIndex = Index;
		}

		checkVoxelSlow(LowestWeight > 0);

		// Move data
		for (int32 Index = LowestIndex + 1; Index < Layers.Num(); Index++)
		{
			Layers[Index - 1] = Layers[Index];
		}

		Layers.Last() = {};
		Layers = Layers.LeftOf(Layers.Num() - 1);
	}

	checkVoxelSlow(Layers.Num() == MaxLayers);
}

void FVoxelSurfaceTypeBlend::PopToMaxLayer_Normalize(const TVoxelArrayView<FVoxelSurfaceTypeBlendLayer> Layers)
{
	VOXEL_FUNCTION_COUNTER();
	checkVoxelSlow(Layers.Num() > MaxLayers);

	PopToMaxLayer_DoNotNormalize(Layers);

	float Multiplier;
	{
		float WeightSum = 0;
		for (int32 Index = 0; Index < MaxLayers; Index++)
		{
			WeightSum += Layers[Index].Weight;
		}
		checkVoxelSlow(WeightSum > 0);

		Multiplier = 1.f / WeightSum;
	}

	for (FVoxelSurfaceTypeBlendLayer& Layer : Layers)
	{
		Layer.Weight *= Multiplier;
	}
}