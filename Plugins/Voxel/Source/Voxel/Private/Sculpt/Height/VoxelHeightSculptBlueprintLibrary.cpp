// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Height/VoxelHeightSculptBlueprintLibrary.h"
#include "Sculpt/Height/VoxelHeightSculptActor.h"

bool UVoxelHeightSculptBlueprintLibrary::IsValidSave(FVoxelHeightSculptSave Save)
{
	return Save.IsValid();
}

bool UVoxelHeightSculptBlueprintLibrary::IsCompressedSave(FVoxelHeightSculptSave Save)
{
	if (!Save.IsValid())
	{
		VOXEL_MESSAGE(Error, "Save is invalid");
		return false;
	}

	return Save.IsCompressed();
}

int64 UVoxelHeightSculptBlueprintLibrary::GetSaveSize(FVoxelHeightSculptSave Save)
{
	if (!Save.IsValid())
	{
		VOXEL_MESSAGE(Error, "Save is invalid");
		return false;
	}

	return Save.GetSize();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFuture UVoxelHeightSculptBlueprintLibrary::ClearSculptData(AVoxelHeightSculptActor* SculptActor)
{
	VOXEL_FUNCTION_COUNTER();

	if (!SculptActor)
	{
		VOXEL_MESSAGE(Error, "SculptActor is invalid");
		return {};
	}

	return SculptActor->ClearSculptData();
}

void UVoxelHeightSculptBlueprintLibrary::ClearSculptCache(AVoxelHeightSculptActor* SculptActor)
{
	VOXEL_FUNCTION_COUNTER();

	if (!SculptActor)
	{
		VOXEL_MESSAGE(Error, "SculptActor is invalid");
		return;
	}

	SculptActor->GetStamp()->ClearCache();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFuture UVoxelHeightSculptBlueprintLibrary::GetSave(
	FVoxelHeightSculptSave& Save,
	AVoxelHeightSculptActor* SculptActor,
	const bool bCompress)
{
	VOXEL_FUNCTION_COUNTER();

	Save = {};

	if (!SculptActor)
	{
		VOXEL_MESSAGE(Error, "SculptActor is null");
		return {};
	}

	return SculptActor->GetSave(bCompress).Then_GameThread([&Save](const TSharedRef<FVoxelHeightSculptSave>& NewSave)
	{
		Save = *NewSave;
	});
}

FVoxelFuture UVoxelHeightSculptBlueprintLibrary::LoadFromSave(
	AVoxelHeightSculptActor* SculptActor,
	const FVoxelHeightSculptSave Save)
{
	VOXEL_FUNCTION_COUNTER();

	if (!SculptActor)
	{
		VOXEL_MESSAGE(Error, "SculptActor is null");
		return {};
	}

	return SculptActor->LoadFromSave(Save);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFuture UVoxelHeightSculptBlueprintLibrary::ApplyModifier(
	AVoxelHeightSculptActor* SculptActor,
	const TSharedRef<FVoxelHeightModifier>& Modifier)
{
	VOXEL_FUNCTION_COUNTER();

	if (!SculptActor)
	{
		VOXEL_MESSAGE(Error, "SculptActor is invalid");
		return {};
	}

	return SculptActor->ApplyModifier(Modifier);
}