// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Graphs/VoxelGraphStamp.h"
#include "VoxelQuery.h"
#include "VoxelStampRef.h"
#include "VoxelHeightStampRef.h"
#include "VoxelVolumeStampRef.h"

EVoxelPinValueOpsUsage FVoxelPinValueOps_FVoxelHeightStampRef::GetUsage() const
{
	return
		EVoxelPinValueOpsUsage::Fixup |
		EVoxelPinValueOpsUsage::CopyFrom;
}

void FVoxelPinValueOps_FVoxelHeightStampRef::Fixup(FVoxelPinValueBase& Value) const
{
	FVoxelHeightStampRef& StampRef = Value.Get<FVoxelHeightStampRef>();
	if (!StampRef.IsValid() ||
		StampRef.IsSharedPtrUnique())
	{
		return;
	}

	// Ensure the stamp ref is unique and not shared across different stamps
	StampRef = StampRef.MakeCopy();
}

void FVoxelPinValueOps_FVoxelHeightStampRef::CopyFrom(
	FVoxelPinValueBase& This,
	const FVoxelPinValueBase& Other) const
{
	FVoxelHeightStampRef& StampRef = This.Get<FVoxelHeightStampRef>();
	const FVoxelHeightStampRef& OtherStampRef = Other.Get<FVoxelHeightStampRef>();

	if (StampRef.GetStruct() != OtherStampRef.GetStruct())
	{
		StampRef = OtherStampRef.MakeCopy();
	}
	else if (StampRef.IsValid())
	{
		// Don't allocate a new shared ptr as that would break graph details
		OtherStampRef.GetStructView().CopyTo(StampRef.GetStructView());
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

EVoxelPinValueOpsUsage FVoxelPinValueOps_FVoxelVolumeStampRef::GetUsage() const
{
	return
		EVoxelPinValueOpsUsage::Fixup |
		EVoxelPinValueOpsUsage::CopyFrom;
}

void FVoxelPinValueOps_FVoxelVolumeStampRef::Fixup(FVoxelPinValueBase& Value) const
{
	FVoxelVolumeStampRef& StampRef = Value.Get<FVoxelVolumeStampRef>();
	if (!StampRef.IsValid() ||
		StampRef.IsSharedPtrUnique())
	{
		return;
	}

	// Ensure the stamp ref is unique and not shared across different stamps
	StampRef = StampRef.MakeCopy();
}

void FVoxelPinValueOps_FVoxelVolumeStampRef::CopyFrom(
	FVoxelPinValueBase& This,
	const FVoxelPinValueBase& Other) const
{
	FVoxelVolumeStampRef& StampRef = This.Get<FVoxelVolumeStampRef>();
	const FVoxelVolumeStampRef& OtherStampRef = Other.Get<FVoxelVolumeStampRef>();

	if (StampRef.GetStruct() != OtherStampRef.GetStruct())
	{
		StampRef = OtherStampRef.MakeCopy();
	}
	else if (StampRef.IsValid())
	{
		// Don't allocate a new shared ptr as that would break graph details
		OtherStampRef.GetStructView().CopyTo(StampRef.GetStructView());
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

EVoxelPinValueOpsUsage FVoxelPinValueOps_FVoxelHeightGraphStampWrapper::GetUsage() const
{
	return
		EVoxelPinValueOpsUsage::GetExposedType |
		EVoxelPinValueOpsUsage::MakeRuntimeValue |
		EVoxelPinValueOpsUsage::HasPinDefaultValue |
		EVoxelPinValueOpsUsage::CustomMetaData;
}

FVoxelPinType FVoxelPinValueOps_FVoxelHeightGraphStampWrapper::GetExposedType() const
{
	return FVoxelPinType::Make<FVoxelHeightStampRef>();
}

FVoxelRuntimePinValue FVoxelPinValueOps_FVoxelHeightGraphStampWrapper::MakeRuntimeValue(
	const FVoxelPinValue& Value,
	const FVoxelPinType::FRuntimeValueContext& Context) const
{
	if (!ensure(Value.Is<FVoxelHeightStampRef>()))
	{
		return {};
	}

	const TSharedPtr<FVoxelStampRuntime> Runtime = FVoxelStampRuntime::Create(
		{},
		Value.Get<FVoxelHeightStampRef>(),
		Context.Component);

	const TSharedRef<FVoxelHeightGraphStampWrapper> GraphStamp = MakeShared<FVoxelHeightGraphStampWrapper>();
	GraphStamp->Stamp = CastStructEnsured<FVoxelHeightStampRuntime>(Runtime);
	return FVoxelRuntimePinValue::Make(GraphStamp);
}

bool FVoxelPinValueOps_FVoxelHeightGraphStampWrapper::HasPinDefaultValue() const
{
	// Storing the stamp in a string is messy
	return false;
}

#if WITH_EDITOR
TMap<FName, FString> FVoxelPinValueOps_FVoxelHeightGraphStampWrapper::GetMetaData() const
{
	return TMap<FName, FString>
	{
		{ "HideStampData", "" }
	};
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

EVoxelPinValueOpsUsage FVoxelPinValueOps_FVoxelVolumeGraphStampWrapper::GetUsage() const
{
	return
		EVoxelPinValueOpsUsage::GetExposedType |
		EVoxelPinValueOpsUsage::MakeRuntimeValue |
		EVoxelPinValueOpsUsage::HasPinDefaultValue |
		EVoxelPinValueOpsUsage::CustomMetaData;
}

FVoxelPinType FVoxelPinValueOps_FVoxelVolumeGraphStampWrapper::GetExposedType() const
{
	return FVoxelPinType::Make<FVoxelVolumeStampRef>();
}

FVoxelRuntimePinValue FVoxelPinValueOps_FVoxelVolumeGraphStampWrapper::MakeRuntimeValue(
	const FVoxelPinValue& Value,
	const FVoxelPinType::FRuntimeValueContext& Context) const
{
	if (!ensure(Value.Is<FVoxelVolumeStampRef>()))
	{
		return {};
	}

	const TSharedPtr<FVoxelStampRuntime> Runtime = FVoxelStampRuntime::Create(
		{},
		Value.Get<FVoxelVolumeStampRef>(),
		Context.Component);

	const TSharedRef<FVoxelVolumeGraphStampWrapper> GraphStamp = MakeShared<FVoxelVolumeGraphStampWrapper>();
	GraphStamp->Stamp = CastStructEnsured<FVoxelVolumeStampRuntime>(Runtime);
	return FVoxelRuntimePinValue::Make(GraphStamp);
}

bool FVoxelPinValueOps_FVoxelVolumeGraphStampWrapper::HasPinDefaultValue() const
{
	// Storing the stamp in a string is messy
	return false;
}

#if WITH_EDITOR
TMap<FName, FString> FVoxelPinValueOps_FVoxelVolumeGraphStampWrapper::GetMetaData() const
{
	return TMap<FName, FString>
	{
		{ "HideStampData", "" }
	};
}
#endif