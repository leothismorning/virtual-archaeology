// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/VoxelChannel.h"
#include "Engine/NetDriver.h"
#include "Engine/NetConnection.h"
#include "Engine/PackageMapClient.h"
#include "Sculpt/Volume/VoxelVolumeSculptActor.h"
#include "Compression/OodleDataCompressionUtil.h"

TVoxelArray<UScriptStruct*> GVoxelRuntimeVolumeTransactionStructs;

VOXEL_RUN_ON_STARTUP_GAME()
{
	for (const UClass* Class : GetDerivedClasses<UNetDriver>())
	{
		UNetDriver& NetDriver = *Class->GetDefaultObject<UNetDriver>();

		FChannelDefinition ChannelDefinition;
		ChannelDefinition.ChannelName = "Voxel";
		ChannelDefinition.ClassName = FName(UVoxelChannel::StaticClass()->GetPathName());
		ChannelDefinition.StaticChannelIndex = -1;
		ChannelDefinition.bTickOnCreate = true;
		ChannelDefinition.bServerOpen = true;
		ChannelDefinition.bClientOpen = true;
		ChannelDefinition.bInitialServer = true;
		ChannelDefinition.bInitialClient = true;
		NetDriver.ChannelDefinitions.Add(ChannelDefinition);
	}

	// TODO
	//GVoxelRuntimeVolumeTransactionStructs = GetDerivedStructs<FVoxelVolumeTransaction>();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UVoxelChannel::UVoxelChannel()
{
	ChName = "Voxel";
}

void UVoxelChannel::Tick()
{
	VOXEL_FUNCTION_COUNTER();

	Super::Tick();

#if 0
	if (!ensure(Connection) ||
		!ensure(Connection->Driver) ||
		!ensure(Connection->Driver->GuidCache) ||
		!ensure(Connection->Channels[ChIndex] == this))
	{
		return;
	}

	if (DataToSend.Num() == 0)
	{
		TVoxelArray<int32> Indices;
		Indices.SetNum(1024 * 1024);

		for (int32 Index = 0; Index < Indices.Num(); Index++)
		{
			Indices[Index] = Index;
		}

		DataToSend = Indices.View<uint8>().Array();
	}

	if (!Connection->Driver->IsServer())
	{
		return;
	}

	if (SendIndex >= DataToSend.Num())
	{
		return;
	}

	if (!Connection->HasReceivedClientPacket())
	{
		return;
	}

	if (SendIndex == 0)
	{
		// Reliable bunch to open channel
		FOutBunch Bunch(this, false);
		Bunch.bReliable = true;
		SendBunch(&Bunch, false);
	}

	for (int32 It = 0; It < 100; It++)
	{
		const int32 PacketSize = FVoxelUtilities::DivideFloor_Positive(Connection->GetMaxSingleBunchSizeBits(), 8);
		const int32 BytesToSend = FMath::Min(PacketSize, DataToSend.Num() - SendIndex);

		FOutBunch Bunch(this, false);

		DataToSend.View().Slice(SendIndex, BytesToSend).Serialize(Bunch);
		SendIndex += BytesToSend;

		GEngine->AddOnScreenDebugMessage(
			0x0394484875_u64,
			1.f,
			FColor::Red,
			"SendIndex: " + FString::FromInt(SendIndex));

		ensure(Bunch.GetNumBytes() <= PacketSize);

		const FPacketIdRange PacketIdRange = SendBunch(&Bunch, false);
		ensure(PacketIdRange.First == PacketIdRange.Last);
	}
#endif
}

void UVoxelChannel::ReceivedBunch(FInBunch& Bunch)
{
	VOXEL_FUNCTION_COUNTER();

	if (Broken ||
		bTornOff)
	{
		return;
	}

	if (!ensure(Connection) ||
		!ensure(Connection->Driver) ||
		!ensure(Connection->Driver->GuidCache) ||
		!ensure(Connection->Channels[ChIndex] == this))
	{
		return;
	}

	TVoxelArray<uint8> CompressedData;
	Bunch << CompressedData;

	TVoxelArray<uint8> Data;
	if (!ensure(FOodleCompressedArray::DecompressToTArray(
		Data,
		CompressedData)))
	{
		return;
	}

	FVoxelReader Reader(Data);

	FNetworkGUID NetGUID;
	Reader << NetGUID;

	UObject* Object = Connection->Driver->GuidCache->GetObjectFromNetGUID(NetGUID, false);
	if (!ensure(Object))
	{
		return;
	}

	AVoxelVolumeSculptActor* Actor = CastEnsured<AVoxelVolumeSculptActor>(Object);
	if (!ensure(Actor))
	{
		return;
	}

	int32 Index = -1;
	Reader << Index;

	if (!ensure(GVoxelRuntimeVolumeTransactionStructs.IsValidIndex(Index)))
	{
		return;
	}

#if 0 // TODO
	const TSharedRef<FVoxelVolumeTransaction> Transaction = MakeSharedStruct<FVoxelVolumeTransaction>(GVoxelRuntimeVolumeTransactionStructs[Index]);
	Transaction->Serialize(Reader.Ar());

	if (!ensure(Reader.IsAtEndWithoutError()))
	{
		return;
	}

	Actor->GetStamp()->ApplyTransaction(
		Transaction,
		Actor->Cache);

	if (Connection->Driver->IsServer())
	{
		OnTransaction(*Actor, Transaction);
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelMap<TVoxelObjectPtr<UNetConnection>, int32> GNetConnectionToVoxelChannel;

UVoxelChannel* UVoxelChannel::FindChannel(UNetConnection& Connection)
{
	VOXEL_FUNCTION_COUNTER();

	int32 Index = -1;
	if (const int32* IndexPtr = GNetConnectionToVoxelChannel.Find(Connection))
	{
		Index = *IndexPtr;
	}

	if (!Connection.Channels.IsValidIndex(Index) ||
		!ensureVoxelSlow(Cast<UVoxelChannel>(Connection.Channels[Index])))
	{
		VOXEL_SCOPE_COUNTER("Iterate Channels");

		for (int32 ChannelIndex = 0; ChannelIndex < Connection.Channels.Num(); ChannelIndex++)
		{
			const TObjectPtr<UChannel> ChannelPtr = Connection.Channels[ChannelIndex];
			if (!ChannelPtr)
			{
				continue;
			}

			const UChannel* Channel = ResolveObjectPtrFast(ChannelPtr);
			if (!Channel ||
				!Channel->IsA<UVoxelChannel>())
			{
				continue;
			}

			for (auto It = GNetConnectionToVoxelChannel.CreateIterator(); It; ++It)
			{
				if (!It.Key().IsValid_Slow())
				{
					It.RemoveCurrent();
				}
			}
			GNetConnectionToVoxelChannel.FindOrAdd(Connection) = ChannelIndex;

			Index = ChannelIndex;
			break;
		}
	}

	if (!Connection.Channels.IsValidIndex(Index) ||
		!ensureVoxelSlow(Cast<UVoxelChannel>(Connection.Channels[Index])))
	{
		return nullptr;
	}

	UVoxelChannel* Channel = CastChecked<UVoxelChannel>(Connection.Channels[Index]);
	ensure(Channel->ChIndex == Index);
	return Channel;
}

TVoxelArray<UVoxelChannel*> UVoxelChannel::GetChannels(const UWorld* World)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensureVoxelSlow(World))
	{
		return {};
	}

	if (World->GetNetMode() == NM_Standalone)
	{
		return {};
	}

	const UNetDriver* NetDriver = World->GetNetDriver();
	if (!NetDriver)
	{
		return {};
	}

	if (World->GetNetMode() == NM_Client)
	{
		const APlayerController* Controller = World->GetFirstPlayerController();
		if (!ensureVoxelSlow(Controller))
		{
			return {};
		}

		UNetConnection* Connection = Controller->GetNetConnection();
		if (!ensureVoxelSlow(Connection))
		{
			return {};
		}

		UVoxelChannel* Channel = FindChannel(*Connection);
		if (!ensureVoxelSlow(Channel))
		{
			return {};
		}

		return { Channel };
	}
	else
	{
		ensure(
			World->GetNetMode() == NM_DedicatedServer ||
			World->GetNetMode() == NM_ListenServer);

		TVoxelArray<UVoxelChannel*> Result;

		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			const APlayerController* Controller = Iterator->Get();
			if (!ensureVoxelSlow(Controller) ||
				Controller->IsLocalController())
			{
				continue;
			}

			UNetConnection* Connection = Controller->GetNetConnection();
			if (!ensureVoxelSlow(Connection))
			{
				continue;
			}

			UVoxelChannel* Channel = FindChannel(*Connection);
			if (!ensureVoxelSlow(Channel))
			{
				continue;
			}

			Result.Add(Channel);
		}

		return Result;
	}
}

#if 0 // TODO
void UVoxelChannel::OnTransaction(
	AVoxelVolumeSculptActor& Actor,
	const TSharedRef<FVoxelVolumeTransaction>& Transaction)
{
	VOXEL_FUNCTION_COUNTER();

	for (UVoxelChannel* Channel : GetChannels(Actor.GetWorld()))
	{
		const UNetConnection* Connection = Channel->Connection;
		if (!ensure(Connection) ||
			!ensure(Connection->Channels[Channel->ChIndex] == Channel))
		{
			continue;
		}

		const UNetDriver* Driver = Connection->Driver;
		if (!ensure(Driver))
		{
			continue;
		}

		const TSharedPtr<FNetGUIDCache> GuidCache = Driver->GuidCache;
		if (!ensure(GuidCache))
		{
			continue;
		}

		FOutBunch Bunch(Channel, false);
		Bunch.bReliable = true;

		{
			FVoxelWriter Writer;

			FNetworkGUID NetGUID = GuidCache->GetOrAssignNetGUID(&Actor);
			Writer << NetGUID;

			int32 Index = GVoxelRuntimeVolumeTransactionStructs.Find(Transaction->GetStruct());
			check(Index != -1);
			Writer << Index;

			Transaction->Serialize(Writer.Ar());

			TVoxelArray<uint8> Data = TVoxelArray<uint8>(Writer.Move());

			TVoxelArray<uint8> CompressedData;
			verify(FOodleCompressedArray::CompressTArray(
				CompressedData,
				Data,
				FOodleDataCompression::ECompressor::Leviathan,
				FOodleDataCompression::ECompressionLevel::Optimal3));

			Bunch << CompressedData;
		}

		Channel->SendBunch(&Bunch, true);
	}
}
#endif