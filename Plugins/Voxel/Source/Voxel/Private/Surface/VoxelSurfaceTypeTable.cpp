// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Surface/VoxelSurfaceTypeTable.h"
#include "Surface/VoxelSmartSurfaceType.h"
#include "Surface/VoxelSurfaceTypeAsset.h"
#include "Surface/VoxelSmartSurfaceProxy.h"
#include "VoxelDependency.h"
#include "VoxelInvalidationCallstack.h"

VOXEL_RUN_ON_STARTUP_GAME()
{
	Voxel::OnRefreshAll.AddLambda([]
	{
		ForEachObjectOfClass_Copy<UVoxelSmartSurfaceType>([&](UVoxelSmartSurfaceType& SmartSurface)
		{
			SmartSurface.GetDependency().Invalidate();
		});

		FVoxelSurfaceTypeTable::Refresh();
	});
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

class FVoxelSurfaceTypeTableManager : public FVoxelSingleton
{
public:
	TSharedPtr<FVoxelSurfaceTypeTable> Table;
	TSharedPtr<FVoxelDependency> InvisibleSurfaceTypesDependency;
	TVoxelMap<TVoxelObjectPtr<UVoxelSmartSurfaceType>, TSharedPtr<FVoxelSmartSurfaceProxy>> SmartSurfaceTypeToProxy;

	TSharedRef<FVoxelSurfaceTypeTable> CreateTable()
	{
		VOXEL_FUNCTION_COUNTER();

		TVoxelMap<FVoxelSurfaceType, TSharedPtr<FVoxelSmartSurfaceProxy>> SurfaceTypeToSmartSurfaceProxy;
		SurfaceTypeToSmartSurfaceProxy.Reserve(32);

		TVoxelArray<FVoxelSurfaceType> InvisibleSurfaceTypes;
		InvisibleSurfaceTypes.Reserve(8);

		FVoxelSurfaceType::ForeachSurfaceType([&](const FVoxelSurfaceType& SurfaceType)
		{
			switch (SurfaceType.GetClass())
			{
			default: VOXEL_ASSUME(false);
			case FVoxelSurfaceType::EClass::SurfaceTypeAsset:
			{
				const UVoxelSurfaceTypeAsset* Type = SurfaceType.GetSurfaceTypeAsset().Resolve();
				if (!ensureVoxelSlow(Type) ||
					!Type->bInvisible)
				{
					return;
				}

				InvisibleSurfaceTypes.Add(SurfaceType);
			}
			break;
			case FVoxelSurfaceType::EClass::SmartSurfaceType:
			{
				UVoxelSmartSurfaceType* Type = SurfaceType.GetSmartSurfaceType().Resolve();
				if (!ensureVoxelSlow(Type))
				{
					return;
				}

				SurfaceTypeToSmartSurfaceProxy.Add_EnsureNew(
					SurfaceType,
					GetProxy(*Type));
			}
			break;
			}
		});

		if (!InvisibleSurfaceTypesDependency)
		{
			InvisibleSurfaceTypesDependency = FVoxelDependency::Create("InvisibleSurfaces");
		}

		return MakeShareable(new FVoxelSurfaceTypeTable(
			MoveTemp(InvisibleSurfaceTypes),
			InvisibleSurfaceTypesDependency.ToSharedRef(),
			MoveTemp(SurfaceTypeToSmartSurfaceProxy)));
	}

	TSharedPtr<FVoxelSmartSurfaceProxy> GetProxy(UVoxelSmartSurfaceType& SurfaceType)
	{
		VOXEL_FUNCTION_COUNTER();
		check(IsInGameThread());

		FVoxelInvalidationScope Scope(SurfaceType);

		TSharedPtr<FVoxelSmartSurfaceProxy>& Proxy = SmartSurfaceTypeToProxy.FindOrAdd(&SurfaceType);
		if (Proxy &&
			!Proxy->DependencyTracker->IsInvalidated())
		{
			return Proxy.ToSharedRef();
		}

		TSharedPtr<FVoxelDependency> Dependency;
		if (Proxy)
		{
			Dependency = Proxy->Dependency;
			Dependency->Invalidate();
		}
		else
		{
			Dependency = FVoxelDependency::Create(SurfaceType.GetPathName());
		}

		FVoxelDependencyCollector DependencyCollector(SurfaceType.GetFName());
		DependencyCollector.AddDependency(SurfaceType.GetDependency());

		const TVoxelNodeEvaluator<FVoxelOutputNode_OutputSurface> Evaluator = SurfaceType.CreateEvaluator(DependencyCollector);

		if (!Evaluator)
		{
			return {};
		}

		Proxy = MakeShareable(new FVoxelSmartSurfaceProxy(
			SurfaceType.GetFName(),
			Dependency.ToSharedRef(),
			Evaluator,
			DependencyCollector.Finalize(nullptr, [](const FVoxelInvalidationCallstack& Callstack)
			{
#if VOXEL_INVALIDATION_TRACKING
				const TSharedRef<const FVoxelInvalidationCallstack> SharedCallstack = Callstack.AsShared();
#endif

				Voxel::GameTask([=]
				{
#if VOXEL_INVALIDATION_TRACKING
					FVoxelInvalidationScope LocalScope(SharedCallstack);
#endif

					FVoxelSurfaceTypeTable::Refresh();
				});
			})));

		return Proxy.ToSharedRef();
	}
};
FVoxelSurfaceTypeTableManager* GVoxelSurfaceTypeTableManager = new FVoxelSurfaceTypeTableManager();

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

TSharedRef<FVoxelSurfaceTypeTable> FVoxelSurfaceTypeTable::Get()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	TSharedPtr<FVoxelSurfaceTypeTable>& Table = GVoxelSurfaceTypeTableManager->Table;
	if (!Table)
	{
		Table = GVoxelSurfaceTypeTableManager->CreateTable();
	}

	return Table.ToSharedRef();
}

void FVoxelSurfaceTypeTable::Refresh()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	FVoxelInvalidationScope Scope("FVoxelSurfaceTable::Refresh");

	const TSharedPtr<FVoxelSurfaceTypeTable> OldTable = GVoxelSurfaceTypeTableManager->Table;

	GVoxelSurfaceTypeTableManager->Table = GVoxelSurfaceTypeTableManager->CreateTable();

	if (OldTable &&
		OldTable->InvisibleSurfaceTypes != GVoxelSurfaceTypeTableManager->Table->InvisibleSurfaceTypes)
	{
		GVoxelSurfaceTypeTableManager->InvisibleSurfaceTypesDependency->Invalidate();
	}
}