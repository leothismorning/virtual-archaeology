// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "MegaMaterial/VoxelMegaMaterialGeneratedData.h"
#include "MegaMaterial/VoxelMegaMaterial.h"
#include "MegaMaterial/VoxelMegaMaterialGenerator.h"
#include "VoxelMaterialUsage.h"
#include "Nanite/VoxelMaterialSelectionCS.h"
#include "Surface/VoxelSurfaceTypeAsset.h"
#include "MegaMaterial/VoxelMegaMaterialCache.h"
#include "MegaMaterial/VoxelMegaMaterialProxy.h"

#if WITH_EDITOR
#include "AssetCompilingManager.h"
#include "IAssetCompilingManager.h"
#include "ComponentRecreateRenderStateContext.h"
#include "UObject/ObjectSaveContext.h"
#include "MaterialEditor/PreviewMaterial.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialInstanceConstant.h"
#endif

#if WITH_EDITOR
class FVoxelMegaMaterialGeneratedDataManager : public FVoxelSingleton
{
public:
	TVoxelMap<TVoxelObjectPtr<UVoxelMegaMaterialGeneratedData>, bool> GeneratedDataToRebuildToIsInteractive;

	//~ Begin FVoxelSingleton Interface
	virtual void Initialize() override
	{
		UMaterial::OnMaterialCompilationFinished().AddLambda([](UMaterialInterface* Material)
		{
			if (Material->IsA<UPreviewMaterial>())
			{
				return;
			}

			UVoxelMegaMaterialGeneratedData::OnMaterialChanged(Material);
		});

		FCoreUObjectDelegates::OnObjectPreSave.AddLambda([](UObject* Object, const FObjectPreSaveContext&)
		{
			if (!Object)
			{
				return;
			}

			if (Object->IsA<UMaterialInstance>())
			{
				UVoxelMegaMaterialGeneratedData::OnMaterialChanged(CastChecked<UMaterialInstance>(Object));
			}

			if (Object->IsA<UMaterialFunction>())
			{
				UVoxelMegaMaterialGeneratedData::OnMaterialFunctionChanged(CastChecked<UMaterialFunction>(Object));
			}
		});

		FCoreUObjectDelegates::OnObjectPropertyChanged.AddLambda([](UObject* Object, const FPropertyChangedEvent& PropertyChangedEvent)
		{
			if (!Object ||
				!Object->IsA<UMaterialInstance>())
			{
				return;
			}

			// Material instance changes don't have a proper Interactive flag set in PropertyChangedEvent
			// Always assume they are interactive, OnObjectPreSave will handle the final flushing
			UVoxelMegaMaterialGeneratedData::OnMaterialChanged(
				CastChecked<UMaterialInstance>(Object),
				true);
		});
	}
	virtual void Tick() override
	{
		VOXEL_FUNCTION_COUNTER();

		if (GeneratedDataToRebuildToIsInteractive.Num() == 0)
		{
			return;
		}

		// Wait for all compilations to finish before rebuilding
		// Not doing this causes GPU crashes & compilation loops
		for (const IAssetCompilingManager* AssetCompilingManager : FAssetCompilingManager::Get().GetRegisteredManagers())
		{
			if (AssetCompilingManager->GetNumRemainingAssets() > 0)
			{
				return;
			}
		}

		for (auto It = GeneratedDataToRebuildToIsInteractive.CreateIterator(); It; ++It)
		{
			UVoxelMegaMaterialGeneratedData* GeneratedData = It.Key().Resolve();
			const bool bInteractive = It.Value();
			It.RemoveCurrent();

			if (!ensureVoxelSlow(GeneratedData))
			{
				continue;
			}

			GeneratedData->RebuildNow(bInteractive);

			// Rebuild one at a time
			break;
		}
	}
	//~ End FVoxelSingleton Interface
};
FVoxelMegaMaterialGeneratedDataManager* GVoxelMegaMaterialGeneratedDataManager = new FVoxelMegaMaterialGeneratedDataManager();
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UVoxelMegaMaterial* UVoxelMegaMaterialGeneratedData::GetMegaMaterial()
{
	VOXEL_FUNCTION_COUNTER();

	if (UVoxelMegaMaterial* MegaMaterial = WeakMegaMaterial.Resolve())
	{
		return MegaMaterial;
	}

	WeakMegaMaterial = SoftMegaMaterial.LoadSynchronous();
	return WeakMegaMaterial.Resolve();
}

void UVoxelMegaMaterialGeneratedData::SetMegaMaterial(UVoxelMegaMaterial* NewMegaMaterial)
{
	SoftMegaMaterial = NewMegaMaterial;
	WeakMegaMaterial = NewMegaMaterial;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelMegaMaterialGeneratedData::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	SerializeVoxelVersion(Ar);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void UVoxelMegaMaterialGeneratedData::ForceRebuild()
{
	IndexToGeneratedMaterial.Reset();
	TargetToGeneratedMaterial.Reset();

	QueueRebuild();
}

void UVoxelMegaMaterialGeneratedData::QueueRebuild(const bool bInteractive)
{
	if (bool* IsInteractivePtr = GVoxelMegaMaterialGeneratedDataManager->GeneratedDataToRebuildToIsInteractive.Find(this))
	{
		*IsInteractivePtr &= bInteractive;
	}
	else
	{
		GVoxelMegaMaterialGeneratedDataManager->GeneratedDataToRebuildToIsInteractive.Add_EnsureNew(this, bInteractive);
	}
}

bool UVoxelMegaMaterialGeneratedData::IsRebuildQueued() const
{
	return GVoxelMegaMaterialGeneratedDataManager->GeneratedDataToRebuildToIsInteractive.Contains(MakeVoxelObjectPtr(ConstCast(this)));
}

void UVoxelMegaMaterialGeneratedData::RebuildNow(const bool bInteractive)
{
	VOXEL_FUNCTION_COUNTER();

	static const bool bNoVoxelMegaMaterial = FParse::Param(FCommandLine::Get(), TEXT("NoVoxelMegaMaterial"));
	if (bNoVoxelMegaMaterial)
	{
		return;
	}

	// Combine all recreates that will be triggered below
	TVoxelOptional<FGlobalComponentRecreateRenderStateContext> Context;
	if (!bInteractive)
	{
		Context.Emplace();
	}

	UVoxelMegaMaterial* MegaMaterial = GetMegaMaterial();
	if (!ensureVoxelSlow(MegaMaterial))
	{
		return;
	}

	ON_SCOPE_EXIT
	{
		if (const UVoxelMegaMaterialCache* Cache = GetTypedOuter<UVoxelMegaMaterialCache>())
		{
			Cache->AutoSaveIfEnabled();
		}

		const TSharedRef<FVoxelMegaMaterialProxy> OldProxy = MegaMaterial->GetProxy();
		const TSharedRef<FVoxelMegaMaterialProxy> NewProxy = MakeShareable(new FVoxelMegaMaterialProxy(*MegaMaterial));
		NewProxy->Initialize(&OldProxy.Get());

		if (NewProxy->Equals(*OldProxy))
		{
			// Only change the proxy if needed
			return;
		}

		MegaMaterial->Proxy = NewProxy;
	};

	IndexToSurfaceInfo.Reset();
	MetadataIndexToMetadata.Reset();
	TargetToMaterial.Reset();

	int32 Counter = 1;
	for (UVoxelSurfaceTypeAsset* SurfaceType : MegaMaterial->SurfaceTypes)
	{
		if (!SurfaceType ||
			!SurfaceType->Material)
		{
			continue;
		}

		const bool bAlreadyAdded = INLINE_LAMBDA
		{
			for (const auto& It : IndexToSurfaceInfo)
			{
				if (It.Value.SurfaceType == SurfaceType)
				{
					return true;
				}
			}

			return false;
		};

		if (bAlreadyAdded)
		{
			continue;
		}

		const UMaterial* BaseMaterial = SurfaceType->Material->GetMaterial();
		if (!BaseMaterial)
		{
			continue;
		}

		FVoxelMegaMaterialSurfaceInfo& MaterialInfo = IndexToSurfaceInfo.Add(FVoxelMaterialRenderIndex(Counter++));
		MaterialInfo.SurfaceType = SurfaceType;

		for (UVoxelMetadata* Metadata : FVoxelMegaMaterialGenerator::GetUsedMetadatas(*BaseMaterial))
		{
			MaterialInfo.UsedMetadatas.Add(Metadata);
			MetadataIndexToMetadata.AddUnique(Metadata);
		}

		if (MegaMaterial->AttributePostProcess)
		{
			// Add PostProcess to every material

			for (UVoxelMetadata* Metadata : FVoxelMegaMaterialGenerator::GetUsedMetadatas(*MegaMaterial->AttributePostProcess))
			{
				MaterialInfo.UsedMetadatas.Add(Metadata);
				MetadataIndexToMetadata.AddUnique(Metadata);
			}
		}
	}

	if (!bInteractive)
	{
		// Reset watched materials, they'll be added back below
		WatchedMaterials.Reset();
		WatchedMaterials.Add(MegaMaterial->AttributePostProcess);
	}

	{
		VOXEL_SCOPE_COUNTER("Generate targets");

		if (MegaMaterial->NonNaniteMaterialType == EVoxelMegaMaterialGenerationType::Custom)
		{
			TargetToMaterial.Add(EVoxelMegaMaterialTarget::NonNanite, MegaMaterial->CustomNonNaniteMaterial);
			TargetToGeneratedMaterial.Remove(EVoxelMegaMaterialTarget::NonNanite);
		}
		else
		{
			GenerateMaterialForTarget(*MegaMaterial, EVoxelMegaMaterialTarget::NonNanite, bInteractive);
		}

		if (MegaMaterial->NaniteDisplacementMaterialType == EVoxelMegaMaterialGenerationType::Custom)
		{
			if (MegaMaterial->CustomNaniteDisplacementMaterial)
			{
				if (!MegaMaterial->CustomNaniteDisplacementMaterial->IsTessellationEnabled())
				{
					VOXEL_MESSAGE(Error, "{0}: NaniteDisplacementMaterial {1} should have tessellation enabled",
						MegaMaterial,
						MegaMaterial->CustomNaniteDisplacementMaterial);
				}

				if (UMaterial* Material = MegaMaterial->CustomNaniteDisplacementMaterial->GetMaterial())
				{
					if (!Material->bUsedWithVoxelMaterialSelection)
					{
						Material->Modify();
						Material->CheckMaterialUsage(MATUSAGE_VirtualHeightfieldMesh);
					}
				}
			}

			TargetToMaterial.Add(EVoxelMegaMaterialTarget::NaniteWPO, MegaMaterial->CustomNaniteDisplacementMaterial);
			TargetToMaterial.Add(EVoxelMegaMaterialTarget::NaniteDisplacement, MegaMaterial->CustomNaniteDisplacementMaterial);
			TargetToMaterial.Add(EVoxelMegaMaterialTarget::NaniteMaterialSelection, MegaMaterial->CustomNaniteDisplacementMaterial);

			TargetToGeneratedMaterial.Remove(EVoxelMegaMaterialTarget::NaniteWPO);
			TargetToGeneratedMaterial.Remove(EVoxelMegaMaterialTarget::NaniteDisplacement);
			TargetToGeneratedMaterial.Remove(EVoxelMegaMaterialTarget::NaniteMaterialSelection);
		}
		else
		{
			GenerateMaterialForTarget(*MegaMaterial, EVoxelMegaMaterialTarget::NaniteWPO, bInteractive);
			GenerateMaterialForTarget(*MegaMaterial, EVoxelMegaMaterialTarget::NaniteDisplacement, bInteractive);
			GenerateMaterialForTarget(*MegaMaterial, EVoxelMegaMaterialTarget::NaniteMaterialSelection, bInteractive);
		}

		if (MegaMaterial->LumenMaterialType == EVoxelMegaMaterialGenerationType::Custom)
		{
			TargetToMaterial.Add(EVoxelMegaMaterialTarget::Lumen, MegaMaterial->CustomLumenMaterial);
			TargetToGeneratedMaterial.Remove(EVoxelMegaMaterialTarget::Lumen);
		}
		else
		{
			GenerateMaterialForTarget(*MegaMaterial, EVoxelMegaMaterialTarget::Lumen, bInteractive);
		}
	}

	{
		VOXEL_SCOPE_COUNTER("Generate materials");

		const TMap<FVoxelMaterialRenderIndex, FVoxelMegaMaterialGeneratedMaterial> OldIndexToGeneratedMaterial = MoveTemp(IndexToGeneratedMaterial);
		check(IndexToGeneratedMaterial.Num() == 0);

		for (const auto& It : IndexToSurfaceInfo)
		{
			FVoxelMegaMaterialGeneratedMaterial& GeneratedMaterial = IndexToGeneratedMaterial.Add(It.Key);

			if (const FVoxelMegaMaterialGeneratedMaterial* OldGeneratedMaterial = OldIndexToGeneratedMaterial.Find(It.Key))
			{
				GeneratedMaterial = *OldGeneratedMaterial;
			}

			GenerateMaterial(
				*MegaMaterial,
				*It.Value.SurfaceType,
				GeneratedMaterial,
				bInteractive);
		}
	}

	// Fixup user-provided materials
	for (auto& It : TargetToMaterial)
	{
		if (!It.Value)
		{
			It.Value = LoadObject<UMaterialInterface>(nullptr, TEXT("/Voxel/Default/VoxelWorldGridMaterial.VoxelWorldGridMaterial"));
			ensure(It.Value);
		}
	}
}

void UVoxelMegaMaterialGeneratedData::OnMaterialChanged(
	UMaterialInterface* Material,
	const bool bInteractive)
{
	VOXEL_FUNCTION_COUNTER();

	ForEachObjectOfClass_Copy<UVoxelMegaMaterialGeneratedData>([&](UVoxelMegaMaterialGeneratedData& GeneratedData)
	{
		if (!GeneratedData.WatchedMaterials.Contains(Material))
		{
			return;
		}

		GeneratedData.QueueRebuild(bInteractive);
	});
}

void UVoxelMegaMaterialGeneratedData::OnMaterialFunctionChanged(UMaterialFunction* MaterialFunction)
{
	VOXEL_FUNCTION_COUNTER();

	ForEachObjectOfClass_Copy<UVoxelMegaMaterialGeneratedData>([&](UVoxelMegaMaterialGeneratedData& GeneratedData)
	{
		if (const UVoxelMegaMaterial* MegaMaterial = GeneratedData.WeakMegaMaterial.Resolve())
		{
			if (MegaMaterial->AttributePostProcess == MaterialFunction)
			{
				GeneratedData.QueueRebuild();
			}
		}
	});
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void UVoxelMegaMaterialGeneratedData::GenerateMaterialForTarget(
	const UVoxelMegaMaterial& MegaMaterial,
	const EVoxelMegaMaterialTarget Target,
	const bool bInteractive)
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelMegaMaterialGeneratedMaterial& GeneratedMaterial = TargetToGeneratedMaterial.FindOrAdd(Target);
	if (!GeneratedMaterial.Material)
	{
		GeneratedMaterial.Material = FVoxelUtilities::NewObject_Safe<UMaterial>(
			this,
			FName(MegaMaterial.GetName() + "_Generated_" + LexToString(Target)));
	}
	if (!GeneratedMaterial.Instance ||
		!ensureVoxelSlow(GeneratedMaterial.Instance->Parent == GeneratedMaterial.Material))
	{
		GeneratedMaterial.Instance = FVoxelUtilities::NewObject_Safe<UMaterialInstanceConstant>(
			this,
			FName(MegaMaterial.GetName() + "_GeneratedInstance_" + LexToString(Target)));

		GeneratedMaterial.Instance->Parent = GeneratedMaterial.Material;
	}

	TargetToMaterial.Add(Target, GeneratedMaterial.Instance);

	switch (Target)
	{
	default: check(false);
	case EVoxelMegaMaterialTarget::NonNanite:
	{
		GeneratedMaterial.Material->bUsedWithNanite = false;
		GeneratedMaterial.Material->bUsedWithVoxelPlugin = true;
	}
	break;
	case EVoxelMegaMaterialTarget::NaniteWPO:
	{
		GeneratedMaterial.Material->bUsedWithNanite = true;
		GeneratedMaterial.Material->bEnableTessellation = false;
	}
	break;
	case EVoxelMegaMaterialTarget::NaniteDisplacement:
	{
		GeneratedMaterial.Material->bUsedWithNanite = true;
		GeneratedMaterial.Material->bEnableTessellation = true;
	}
	break;
	case EVoxelMegaMaterialTarget::NaniteMaterialSelection:
	{
		GeneratedMaterial.Material->bUsedWithNanite = false;
		GeneratedMaterial.Material->bEnableTessellation = true;
		GeneratedMaterial.Material->bUsedWithVoxelMaterialSelection = true;
	}
	break;
	case EVoxelMegaMaterialTarget::Lumen:
	{
		GeneratedMaterial.Material->bUsedWithNanite = false;
		GeneratedMaterial.Material->bUsedWithVoxelPlugin = true;
	}
	break;
	}

	const auto UpdateMaterialInstance = [&]
	{
		FMaterialInstanceParameterUpdateContext UpdateContext(GeneratedMaterial.Instance);

		bool bChanged = false;
		for (const auto& It : IndexToSurfaceInfo)
		{
			bChanged |= FVoxelUtilities::CopyParameterValues(
				UpdateContext,
				*GeneratedMaterial.Instance,
				*It.Value.SurfaceType->Material,
				"VOXELMATERIAL_" + FString::FromInt(It.Key.Index) + "_");
		}

		bChanged |= FVoxelMegaMaterialGenerator::ApplyBlendSmoothness(
			IndexToSurfaceInfo,
			*GeneratedMaterial.Material,
			*GeneratedMaterial.Instance);

		if (bChanged)
		{
			GeneratedMaterial.Instance->PostEditChange();
		}
	};

	if (bInteractive)
	{
		UpdateMaterialInstance();
		return;
	}

	if (!IsRunningCookCommandlet())
	{
		const double StartTime = FPlatformTime::Seconds();

		UMaterial* DummyMaterial = NewObject<UMaterial>();
		ON_SCOPE_EXIT
		{
			// Ensure the dummy material isn't compiled
			FVoxelUtilities::ClearMaterialExpressions(*DummyMaterial);
			DummyMaterial->CancelOutstandingCompilation();
			DummyMaterial->ConditionalBeginDestroy();
		};

		// Copy flags to not fail diffing on them
		DummyMaterial->bUsedWithNanite = GeneratedMaterial.Material->bUsedWithNanite;
		DummyMaterial->bEnableTessellation = GeneratedMaterial.Material->bEnableTessellation;
		DummyMaterial->bUsedWithVoxelPlugin = GeneratedMaterial.Material->bUsedWithVoxelPlugin;
		DummyMaterial->bUsedWithVoxelMaterialSelection = GeneratedMaterial.Material->bUsedWithVoxelMaterialSelection;

		UMaterialInstanceConstant* DummyMaterialInstance = NewObject<UMaterialInstanceConstant>();
		DummyMaterialInstance->Parent = DummyMaterial;
		ON_SCOPE_EXIT
		{
			DummyMaterialInstance->ConditionalBeginDestroy();
		};

		if (!ensureVoxelSlow(FVoxelMegaMaterialGenerator::GenerateMaterialForTarget(
			MegaMaterial,
			IndexToSurfaceInfo,
			Target,
			*DummyMaterial,
			*DummyMaterialInstance,
			MetadataIndexToMetadata,
			WatchedMaterials,
			true)))
		{
			return;
		}

		FString MaterialDiff;
		const bool bSameMaterial = FVoxelUtilities::AreMaterialsIdentical(
			*GeneratedMaterial.Material,
			*DummyMaterial,
			MaterialDiff);

		const double EndTime = FPlatformTime::Seconds();

		if (bSameMaterial)
		{
			UpdateMaterialInstance();
			return;
		}

		LOG_VOXEL(Log, "Generating MegaMaterial %s target %s took %s, material changes detected. Regenerating. Changes: %s",
			*MegaMaterial.GetPathName(),
			LexToString(Target),
			*FVoxelUtilities::SecondsToString(EndTime - StartTime),
			*MaterialDiff);
	}

	const double StartTime = FPlatformTime::Seconds();

	if (!ensure(FVoxelMegaMaterialGenerator::GenerateMaterialForTarget(
		MegaMaterial,
		IndexToSurfaceInfo,
		Target,
		*GeneratedMaterial.Material,
		*GeneratedMaterial.Instance,
		MetadataIndexToMetadata,
		WatchedMaterials,
		false)))
	{
		return;
	}

	const double EndTime = FPlatformTime::Seconds();

	LOG_VOXEL(Display, "Regenerating MegaMaterial %s target %s took %s",
		*MegaMaterial.GetPathName(),
		LexToString(Target),
		*FVoxelUtilities::SecondsToString(EndTime - StartTime));
}

void UVoxelMegaMaterialGeneratedData::GenerateMaterial(
	const UVoxelMegaMaterial& MegaMaterial,
	const UVoxelSurfaceTypeAsset& SurfaceType,
	FVoxelMegaMaterialGeneratedMaterial& GeneratedMaterial,
	const bool bInteractive)
{
	VOXEL_FUNCTION_COUNTER();

	const UMaterialInterface& MaterialInterface = *SurfaceType.Material;

	if (!GeneratedMaterial.Material)
	{
		GeneratedMaterial.Material = FVoxelUtilities::NewObject_Safe<UMaterial>(
			this,
			FName(MegaMaterial.GetName() + "_Generated_" + MaterialInterface.GetName()));
	}
	if (!GeneratedMaterial.Instance ||
		!ensureVoxelSlow(GeneratedMaterial.Instance->Parent == GeneratedMaterial.Material))
	{
		GeneratedMaterial.Instance = FVoxelUtilities::NewObject_Safe<UMaterialInstanceConstant>(
			this,
			FName(MegaMaterial.GetName() + "_GeneratedInstance_" + MaterialInterface.GetName()));

		GeneratedMaterial.Instance->Parent = GeneratedMaterial.Material;
	}

	if (bInteractive)
	{
		FMaterialInstanceParameterUpdateContext UpdateContext(GeneratedMaterial.Instance);

		if (FVoxelUtilities::CopyParameterValues(
			UpdateContext,
			*GeneratedMaterial.Instance,
			MaterialInterface,
			{}))
		{
			GeneratedMaterial.Instance->PostEditChange();
		}

		return;
	}

	if (!IsRunningCookCommandlet())
	{
		const double StartTime = FPlatformTime::Seconds();

		UMaterial* DummyMaterial = NewObject<UMaterial>();
		ON_SCOPE_EXIT
		{
			// Ensure the dummy material isn't compiled
			FVoxelUtilities::ClearMaterialExpressions(*DummyMaterial);
			DummyMaterial->CancelOutstandingCompilation();
			DummyMaterial->ConditionalBeginDestroy();
		};

		// Copy flags to not fail diffing on them
		DummyMaterial->bUsedWithNanite = GeneratedMaterial.Material->bUsedWithNanite;
		DummyMaterial->bEnableTessellation = GeneratedMaterial.Material->bEnableTessellation;
		DummyMaterial->bUsedWithVoxelPlugin = GeneratedMaterial.Material->bUsedWithVoxelPlugin;
		DummyMaterial->bUsedWithVoxelMaterialSelection = GeneratedMaterial.Material->bUsedWithVoxelMaterialSelection;

		UMaterialInstanceConstant* DummyMaterialInstance = NewObject<UMaterialInstanceConstant>();
		DummyMaterialInstance->Parent = DummyMaterial;
		ON_SCOPE_EXIT
		{
			DummyMaterialInstance->ConditionalBeginDestroy();
		};

		if (!ensureVoxelSlow(FVoxelMegaMaterialGenerator::GenerateMaterial(
			MegaMaterial,
			SurfaceType,
			*DummyMaterial,
			*DummyMaterialInstance,
			MetadataIndexToMetadata,
			true)))
		{
			return;
		}

		FString MaterialDiff;
		const bool bSameMaterial = FVoxelUtilities::AreMaterialsIdentical(
			*GeneratedMaterial.Material,
			*DummyMaterial,
			MaterialDiff);

		const double EndTime = FPlatformTime::Seconds();

		if (bSameMaterial)
		{
			FString InstanceDiff;
			const bool bSameMaterialInstance = FVoxelUtilities::AreInstancesIdentical(
				*GeneratedMaterial.Instance,
				*DummyMaterialInstance,
				InstanceDiff);

			if (bSameMaterialInstance)
			{
				LOG_VOXEL(Verbose, "Generating MegaMaterial %s material %s took %s, no changes detected",
					*MegaMaterial.GetPathName(),
					*MaterialInterface.GetName(),
					*FVoxelUtilities::SecondsToString(EndTime - StartTime));

				return;
			}

			LOG_VOXEL(Log, "Generating MegaMaterial %s material %s took %s, instance changes detected. Updating. Changes: %s",
				*MegaMaterial.GetPathName(),
				*MaterialInterface.GetName(),
				*FVoxelUtilities::SecondsToString(EndTime - StartTime),
				*InstanceDiff);

			GeneratedMaterial.Instance->CopyMaterialUniformParametersEditorOnly(DummyMaterialInstance);
			GeneratedMaterial.Instance->PostEditChange();
			return;
		}

		LOG_VOXEL(Log, "Generating MegaMaterial %s material %s took %s, material changes detected. Regenerating. Changes: %s",
			*MegaMaterial.GetPathName(),
			*MaterialInterface.GetName(),
			*FVoxelUtilities::SecondsToString(EndTime - StartTime),
			*MaterialDiff);
	}

	const double StartTime = FPlatformTime::Seconds();

	if (!ensure(FVoxelMegaMaterialGenerator::GenerateMaterial(
		MegaMaterial,
		SurfaceType,
		*GeneratedMaterial.Material,
		*GeneratedMaterial.Instance,
		MetadataIndexToMetadata,
		false)))
	{
		return;
	}

	const double EndTime = FPlatformTime::Seconds();

	LOG_VOXEL(Display, "Regenerating MegaMaterial %s material %s took %s",
		*MegaMaterial.GetPathName(),
		*MaterialInterface.GetName(),
		*FVoxelUtilities::SecondsToString(EndTime - StartTime));
}
#endif