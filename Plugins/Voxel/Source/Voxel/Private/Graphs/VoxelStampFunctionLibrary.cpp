// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Graphs/VoxelStampFunctionLibrary.h"
#include "Graphs/VoxelStampGraphParameters.h"
#include "VoxelQuery.h"
#include "VoxelLayers.h"
#include "VoxelGraphMigration.h"

VOXEL_RUN_ON_STARTUP_GAME()
{
	REGISTER_VOXEL_FUNCTION_MIGRATION("BlendMaterials", UVoxelStampFunctionLibrary, BlendSurfaceTypes);
	REGISTER_VOXEL_FUNCTION_MIGRATION("GetHeightSmoothness", UVoxelStampFunctionLibrary, GetHeightSmoothness);
	REGISTER_VOXEL_FUNCTION_MIGRATION("GetHeightBlendMode", UVoxelStampFunctionLibrary, GetHeightBlendMode);
	REGISTER_VOXEL_FUNCTION_MIGRATION("IsHeightOverrideBlendMode", UVoxelStampFunctionLibrary, IsHeightOverrideBlendMode);
	REGISTER_VOXEL_FUNCTION_MIGRATION("GetVolumeSmoothness", UVoxelStampFunctionLibrary, GetVolumeSmoothness);
	REGISTER_VOXEL_FUNCTION_MIGRATION("GetVolumeBlendMode", UVoxelStampFunctionLibrary, GetVolumeBlendMode);
	REGISTER_VOXEL_FUNCTION_MIGRATION("IsVolumeOverrideBlendMode", UVoxelStampFunctionLibrary, IsVolumeOverrideBlendMode);
	REGISTER_VOXEL_FUNCTION_MIGRATION("QueryHeight", UVoxelStampFunctionLibrary, QueryHeight);
	REGISTER_VOXEL_FUNCTION_MIGRATION("QueryDistance", UVoxelStampFunctionLibrary, QueryDistance);
	REGISTER_VOXEL_FUNCTION_MIGRATION("QueryHeightMaterial", UVoxelStampFunctionLibrary, QueryHeightSurfaceType);
	REGISTER_VOXEL_FUNCTION_MIGRATION("QueryVolumeMaterial", UVoxelStampFunctionLibrary, QueryVolumeSurfaceType);
	REGISTER_VOXEL_FUNCTION_MIGRATION("QueryHeightMetadata", UVoxelStampFunctionLibrary, QueryHeightMetadata);
	REGISTER_VOXEL_FUNCTION_MIGRATION("QueryVolumeMetadata", UVoxelStampFunctionLibrary, QueryVolumeMetadata);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelSurfaceTypeBlendBuffer UVoxelStampFunctionLibrary::BlendSurfaceTypes(
	const FVoxelSurfaceTypeBlendBuffer& A,
	const FVoxelSurfaceTypeBlendBuffer& B,
	const FVoxelFloatBuffer& Alpha) const
{
	const int32 Num = ComputeVoxelBuffersNum_Return(A, B, Alpha);

	FVoxelSurfaceTypeBlendBuffer Result;
	Result.Allocate(Num);

	for (int32 Index = 0; Index < Num; Index++)
	{
		FVoxelSurfaceTypeBlend::Lerp(
			Result.View()[Index],
			A[Index],
			B[Index],
			FMath::Clamp(Alpha[Index], 0.f, 1.f));
	}

	return Result;
}

FVoxelFloatBuffer UVoxelStampFunctionLibrary::GetSurfaceTypeWeight(
	const FVoxelSurfaceTypeBlendBuffer& Blend,
	const FVoxelSurfaceTypeBuffer& Type) const
{
	const int32 Num = ComputeVoxelBuffersNum_Return(Blend, Type);

	FVoxelFloatBuffer Result;
	Result.Allocate(Num);

	for (int32 Index = 0; Index < Num; Index++)
	{
		const FVoxelSurfaceType TypeToFind = Type[Index];

		const float Weight = INLINE_LAMBDA
		{
			for (const FVoxelSurfaceTypeBlendLayer& Layer : Blend[Index].GetLayers())
			{
				if (Layer.Type == TypeToFind)
				{
					return Layer.Weight;
				}
			}

			return 0.f;
		};

		Result.Set(Index, Weight);
	}

	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

float UVoxelStampFunctionLibrary::GetHeightSmoothness() const
{
	if (Query.IsPreview())
	{
		return 0.f;
	}

	const FVoxelGraphParameters::FHeightStamp* Parameter = Query->FindParameter<FVoxelGraphParameters::FHeightStamp>();
	if (!ensure(Parameter))
	{
		return 0.f;
	}

	return Parameter->Smoothness;
}

EVoxelHeightBlendMode UVoxelStampFunctionLibrary::GetHeightBlendMode() const
{
	if (Query.IsPreview())
	{
		return {};
	}

	const FVoxelGraphParameters::FHeightStamp* Parameter = Query->FindParameter<FVoxelGraphParameters::FHeightStamp>();
	if (!ensure(Parameter))
	{
		return {};
	}

	return Parameter->BlendMode;
}

bool UVoxelStampFunctionLibrary::IsHeightOverrideBlendMode() const
{
	return GetHeightBlendMode() == EVoxelHeightBlendMode::Override;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

float UVoxelStampFunctionLibrary::GetVolumeSmoothness() const
{
	if (Query.IsPreview())
	{
		return 0.f;
	}

	const FVoxelGraphParameters::FVolumeStamp* Parameter = Query->FindParameter<FVoxelGraphParameters::FVolumeStamp>();
	if (!ensure(Parameter))
	{
		return 0.f;
	}

	return Parameter->Smoothness;
}

EVoxelVolumeBlendMode UVoxelStampFunctionLibrary::GetVolumeBlendMode() const
{
	if (Query.IsPreview())
	{
		return {};
	}

	const FVoxelGraphParameters::FVolumeStamp* Parameter = Query->FindParameter<FVoxelGraphParameters::FVolumeStamp>();
	if (!ensure(Parameter))
	{
		return {};
	}

	return Parameter->BlendMode;
}

bool UVoxelStampFunctionLibrary::IsVolumeOverrideBlendMode() const
{
	return GetVolumeBlendMode() == EVoxelVolumeBlendMode::Override;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelStampFunctionLibrary::QueryHeight(
	const FVoxelDoubleVector2DBuffer& Position,
	const FVoxelWeakStackHeightLayer& Layer,
	FVoxelFloatBuffer& Height,
	bool& bIsValid) const
{
	if (Query.IsPreview())
	{
		Height = 0.f;
		bIsValid = true;
		return;
	}

	const FVoxelGraphParameters::FQuery* Parameter = Query->FindParameter<FVoxelGraphParameters::FQuery>();
	if (!Parameter)
	{
		VOXEL_MESSAGE(Error, "{0}: Cannot call here", this);
		return;
	}
	const FVoxelQuery& VoxelQuery = Parameter->Query;

	if (!VoxelQuery.CheckNoRecursion(Layer) ||
		!VoxelQuery.Layers.HasLayer(Layer, Query->Context.DependencyCollector))
	{
		return;
	}

	Height = VoxelQuery.SampleHeightLayer(Layer, Position);

	bIsValid = true;
}

void UVoxelStampFunctionLibrary::QueryDistance(
	const FVoxelDoubleVectorBuffer& Position,
	const FVoxelWeakStackVolumeLayer& Layer,
	FVoxelFloatBuffer& Distance,
	bool& bIsValid) const
{
	if (Query.IsPreview())
	{
		Distance = 1e9f;
		bIsValid = true;
		return;
	}

	const FVoxelGraphParameters::FQuery* Parameter = Query->FindParameter<FVoxelGraphParameters::FQuery>();
	if (!Parameter)
	{
		VOXEL_MESSAGE(Error, "{0}: Cannot call here", this);
		return;
	}
	const FVoxelQuery& VoxelQuery = Parameter->Query;

	if (!VoxelQuery.CheckNoRecursion(Layer) ||
		!VoxelQuery.Layers.HasLayer(Layer, Query->Context.DependencyCollector))
	{
		return;
	}

	Distance = VoxelQuery.SampleVolumeLayer(Layer, Position);

	bIsValid = true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelStampFunctionLibrary::QueryHeightSurfaceType(
	const FVoxelDoubleVector2DBuffer& Position,
	const FVoxelWeakStackHeightLayer& Layer,
	FVoxelFloatBuffer& Height,
	FVoxelSurfaceTypeBlendBuffer& SurfaceType,
	bool& bIsValid) const
{
	if (Query.IsPreview())
	{
		Height = 0.f;
		bIsValid = true;
		return;
	}

	const FVoxelGraphParameters::FQuery* Parameter = Query->FindParameter<FVoxelGraphParameters::FQuery>();
	if (!Parameter)
	{
		VOXEL_MESSAGE(Error, "{0}: Cannot call here", this);
		return;
	}
	const FVoxelQuery& VoxelQuery = Parameter->Query;

	if (!VoxelQuery.CheckNoRecursion(Layer) ||
		!VoxelQuery.Layers.HasLayer(Layer, Query->Context.DependencyCollector))
	{
		return;
	}

	FVoxelDoubleVector2DBuffer SafePositions = Position;
	SafePositions.ExpandConstants();

	SurfaceType.AllocateZeroed(SafePositions.Num());

	Height = VoxelQuery.SampleHeightLayer(
		Layer,
		SafePositions,
		SurfaceType.View(),
		{});

	bIsValid = true;
}

void UVoxelStampFunctionLibrary::QueryVolumeSurfaceType(
	const FVoxelDoubleVectorBuffer& Position,
	const FVoxelWeakStackVolumeLayer& Layer,
	FVoxelFloatBuffer& Distance,
	FVoxelSurfaceTypeBlendBuffer& SurfaceType,
	bool& bIsValid) const
{
	if (Query.IsPreview())
	{
		Distance = 1e9f;
		bIsValid = true;
		return;
	}

	const FVoxelGraphParameters::FQuery* Parameter = Query->FindParameter<FVoxelGraphParameters::FQuery>();
	if (!Parameter)
	{
		VOXEL_MESSAGE(Error, "{0}: Cannot call here", this);
		return;
	}
	const FVoxelQuery& VoxelQuery = Parameter->Query;

	if (!VoxelQuery.CheckNoRecursion(Layer) ||
		!VoxelQuery.Layers.HasLayer(Layer, Query->Context.DependencyCollector))
	{
		return;
	}

	FVoxelDoubleVectorBuffer SafePositions = Position;
	SafePositions.ExpandConstants();

	SurfaceType.AllocateZeroed(SafePositions.Num());

	Distance = VoxelQuery.SampleVolumeLayer(
		Layer,
		SafePositions,
		SurfaceType.View(),
		{});

	bIsValid = true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelStampFunctionLibrary::QueryHeightMetadata(
	const FVoxelDoubleVector2DBuffer& Position,
	const FVoxelWeakStackHeightLayer& Layer,
	const FVoxelFloatMetadataRef& Metadata,
	FVoxelFloatBuffer& Height,
	FVoxelFloatBuffer& Value,
	bool& bIsValid) const
{
	if (Query.IsPreview())
	{
		Height = 0.f;
		bIsValid = true;
		return;
	}

	if (!Metadata.IsValid())
	{
		VOXEL_MESSAGE(Error, "{0}: Metadata is null", this);
		return;
	}

	const FVoxelGraphParameters::FQuery* Parameter = Query->FindParameter<FVoxelGraphParameters::FQuery>();
	if (!Parameter)
	{
		VOXEL_MESSAGE(Error, "{0}: Cannot call here", this);
		return;
	}
	const FVoxelQuery& VoxelQuery = Parameter->Query;

	if (!VoxelQuery.CheckNoRecursion(Layer) ||
		!VoxelQuery.Layers.HasLayer(Layer, Query->Context.DependencyCollector))
	{
		return;
	}

	FVoxelDoubleVector2DBuffer SafePositions = Position;
	SafePositions.ExpandConstants();

	Value.AllocateZeroed(Position.Num());

	TVoxelMap<FVoxelMetadataRef, TSharedRef<FVoxelBuffer>> MetadataToBuffer;
	MetadataToBuffer.Add_EnsureNew(Metadata, MakeSharedCopy(Value));

	Height = VoxelQuery.SampleHeightLayer(
		Layer,
		Position,
		{},
		MetadataToBuffer);

	bIsValid = true;
}

void UVoxelStampFunctionLibrary::QueryVolumeMetadata(
	const FVoxelDoubleVectorBuffer& Position,
	const FVoxelWeakStackVolumeLayer& Layer,
	const FVoxelFloatMetadataRef& Metadata,
	FVoxelFloatBuffer& Distance,
	FVoxelFloatBuffer& Value,
	bool& bIsValid) const
{
	if (Query.IsPreview())
	{
		Distance = 1e9f;
		bIsValid = true;
		return;
	}

	if (!Metadata.IsValid())
	{
		VOXEL_MESSAGE(Error, "{0}: Metadata is null", this);
		return;
	}

	const FVoxelGraphParameters::FQuery* Parameter = Query->FindParameter<FVoxelGraphParameters::FQuery>();
	if (!Parameter)
	{
		VOXEL_MESSAGE(Error, "{0}: Cannot call here", this);
		return;
	}
	const FVoxelQuery& VoxelQuery = Parameter->Query;

	if (!VoxelQuery.CheckNoRecursion(Layer) ||
		!VoxelQuery.Layers.HasLayer(Layer, Query->Context.DependencyCollector))
	{
		return;
	}

	FVoxelDoubleVectorBuffer SafePositions = Position;
	SafePositions.ExpandConstants();

	Value.AllocateZeroed(Position.Num());

	TVoxelMap<FVoxelMetadataRef, TSharedRef<FVoxelBuffer>> MetadataToBuffer;
	MetadataToBuffer.Add_EnsureNew(Metadata, MakeSharedCopy(Value));

	Distance = VoxelQuery.SampleVolumeLayer(
		Layer,
		SafePositions,
		{},
		MetadataToBuffer);

	bIsValid = true;
}