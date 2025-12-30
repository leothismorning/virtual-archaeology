// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Engine/ActorChannel.h"
#include "VoxelChannel.generated.h"

UCLASS()
class VOXEL_API UVoxelChannel : public UChannel
{
	GENERATED_BODY()

public:
	UVoxelChannel();

	//~ Begin UChannel Interface
	virtual bool CanStopTicking() const override
	{
		return false;
	}

	virtual void Tick() override;
	virtual void ReceivedBunch(FInBunch& Bunch) override;
	//~ End UChannel Interface

public:
	int32 SendIndex = 0;
	TVoxelArray<uint8> DataToSend;

	static UVoxelChannel* FindChannel(UNetConnection& Connection);

	static TVoxelArray<UVoxelChannel*> GetChannels(const UWorld* World);

#if 0 // TODO
	static void OnTransaction(
		AVoxelVolumeSculptActor& Actor,
		const TSharedRef<FVoxelVolumeTransaction>& Transaction);
#endif
};