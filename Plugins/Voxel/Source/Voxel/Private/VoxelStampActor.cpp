// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelStampActor.h"
#include "VoxelStampComponent.h"

AVoxelStampActor::AVoxelStampActor()
{
	StampComponent = CreateDefaultSubobject<UVoxelStampComponent>("Stamp");
	RootComponent = StampComponent;
}

UVoxelStampComponent& AVoxelStampActor::GetStampComponent() const
{
	check(StampComponent);
	return *StampComponent;
}

void AVoxelStampActor::PostLoad()
{
	Super::PostLoad();

	if (RootComponent != StampComponent)
	{
		StampComponent->SetRelativeLocation(RootComponent->GetRelativeLocation());
		StampComponent->SetRelativeRotation(RootComponent->GetRelativeRotation());
		StampComponent->SetRelativeScale3D(RootComponent->GetRelativeScale3D());

		RootComponent = StampComponent;
	}
}

void AVoxelStampActor::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	SerializeVoxelVersion(Ar);
}

#if WITH_EDITOR
void AVoxelStampActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property == &FindFPropertyChecked_ByName(AActor, "ActorLabel"))
	{
		GetStampComponent().OnActorLabelChanged();
	}

	// PostEditChangeProperty will reregister and call UpdateStampActorLabel, call it last
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void AVoxelStampActor::PostDuplicate(const EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

	if (DuplicateMode != EDuplicateMode::Normal ||
		!StampComponent ||
		!StampComponent->GetStamp().IsValid())
	{
		return;
	}

	// Do not update priorities if there's more than one stamp component
	int32 NumStampComponents = 0;
	for (UActorComponent* Component : GetComponents())
	{
		if (Cast<UVoxelStampComponent>(Component))
		{
			NumStampComponents++;
			if (NumStampComponents > 1)
			{
				return;
			}
		}
	}

	int32 NewPriority = 0;
	ForEachObjectOfClass<UVoxelStampComponent>([&](const UVoxelStampComponent& Component)
	{
		if (GetWorld() != Component.GetWorld())
		{
			return;
		}

		const FVoxelStampRef& OtherStamp = Component.GetStamp();
		if (!OtherStamp)
		{
			return;
		}

		NewPriority = FMath::Max(NewPriority, OtherStamp->Priority + 1);
	});

	StampComponent->GetStamp()->Priority = NewPriority++;
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelStampRef AVoxelStampActor::GetStamp() const
{
	return GetStampComponent().GetStamp();
}

void AVoxelStampActor::SetStamp(const FVoxelStampRef& NewStamp)
{
	GetStampComponent().SetStamp(NewStamp);
}

void AVoxelStampActor::SetStamp(const FVoxelStamp& NewStamp)
{
	GetStampComponent().SetStamp(NewStamp);
}

void AVoxelStampActor::UpdateStamp()
{
	GetStampComponent().UpdateStamp();
}