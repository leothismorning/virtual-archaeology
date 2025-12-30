// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelStampRef.h"
#include "VoxelStampRuntime.h"
#include "VoxelStampCustomization.h"
#include "VoxelParameterOverridesDetails.h"

template<typename T>
class TVoxelGraphStampCustomization : public FVoxelStampCustomization
{
public:
	virtual void CustomizeChildren(
		const TSharedRef<IPropertyHandle> PropertyHandle,
		IDetailChildrenBuilder& ChildBuilder,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override
	{
		FVoxelStampCustomization::CustomizeChildren(PropertyHandle, ChildBuilder, CustomizationUtils);

		const auto GatherOwners = [&](TVoxelArray<FVoxelParameterOverridesDetails::FWeakOwner>& OutOwners)
		{
			FVoxelEditorUtilities::ForeachData<T>(PropertyHandle, [&](T& Stamp)
			{
				OutOwners.Add(FVoxelParameterOverridesDetails::FWeakOwner
					{
						&Stamp,
						[StampRef = Stamp.GetStampRef()]() -> uint64
						{
							const TSharedPtr<const FVoxelStampRuntime> Runtime = StampRef.ResolveStampRuntime();
							if (!Runtime)
							{
								return 0;
							}
							return Runtime->GetPropertyHash();
						},
						[StampRef = Stamp.GetStampRef()](FVoxelDependencyCollector& DependencyCollector) -> TSharedPtr<FVoxelGraphEnvironment>
						{
							const TSharedPtr<const FVoxelStampRuntime> Runtime = StampRef.ResolveStampRuntime();
							if (!Runtime)
							{
								return nullptr;
							}
							return StampRef.template AsChecked<T>().CreateEnvironment(*Runtime, DependencyCollector);
						},
						PropertyHandle
					});
			});
		};

		KeepAlive(FVoxelParameterOverridesDetails::Create(
			ChildBuilder,
			GatherOwners,
			FVoxelEditorUtilities::MakeRefreshDelegate(this, CustomizationUtils),
			"Parameters"));
	}
};