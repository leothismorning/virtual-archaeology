// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/Volume/VoxelVolumeSculptBlueprintLibrary.h"
#include "Sculpt/Volume/VoxelVolumeSculptActor.h"

bool UVoxelVolumeSculptBlueprintLibrary::IsValidSave(FVoxelVolumeSculptSave Save)
{
	return Save.IsValid();
}

bool UVoxelVolumeSculptBlueprintLibrary::IsCompressedSave(FVoxelVolumeSculptSave Save)
{
	if (!Save.IsValid())
	{
		VOXEL_MESSAGE(Error, "Save is invalid");
		return false;
	}

	return Save.IsCompressed();
}

int64 UVoxelVolumeSculptBlueprintLibrary::GetSaveSize(FVoxelVolumeSculptSave Save)
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

FVoxelFuture UVoxelVolumeSculptBlueprintLibrary::ClearSculptData(AVoxelVolumeSculptActor* SculptActor)
{
	VOXEL_FUNCTION_COUNTER();

	if (!SculptActor)
	{
		VOXEL_MESSAGE(Error, "SculptActor is invalid");
		return {};
	}

	return SculptActor->ClearSculptData();
}

void UVoxelVolumeSculptBlueprintLibrary::ClearSculptCache(AVoxelVolumeSculptActor* SculptActor)
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

FVoxelFuture UVoxelVolumeSculptBlueprintLibrary::GetSave(
	FVoxelVolumeSculptSave& Save,
	AVoxelVolumeSculptActor* SculptActor,
	const bool bCompress)
{
	VOXEL_FUNCTION_COUNTER();

	Save = {};

	if (!SculptActor)
	{
		VOXEL_MESSAGE(Error, "SculptActor is null");
		return {};
	}

	return SculptActor->GetSave(bCompress).Then_GameThread([&Save](const TSharedRef<FVoxelVolumeSculptSave>& NewSave)
	{
		Save = *NewSave;
	});
}

FVoxelFuture UVoxelVolumeSculptBlueprintLibrary::LoadFromSave(
	AVoxelVolumeSculptActor* SculptActor,
	const FVoxelVolumeSculptSave Save)
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

FVoxelFuture UVoxelVolumeSculptBlueprintLibrary::ApplyModifier(
	AVoxelVolumeSculptActor* SculptActor,
	const TSharedRef<FVoxelVolumeModifier>& Modifier)
{
	VOXEL_FUNCTION_COUNTER();

	if (!SculptActor)
	{
		VOXEL_MESSAGE(Error, "SculptActor is invalid");
		return {};
	}

	return SculptActor->ApplyModifier(Modifier);
}