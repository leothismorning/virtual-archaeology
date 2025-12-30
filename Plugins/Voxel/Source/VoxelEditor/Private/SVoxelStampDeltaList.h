// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelStampDelta.h"

class SVoxelStampDeltaListEntry : public SMultiColumnTableRow<TSharedPtr<FVoxelStampDelta>>
{
public:
	VOXEL_SLATE_ARGS()
	{
	};

	void Construct(
		const FArguments& Args,
		const TSharedRef<STableViewBase>& OwnerTableView,
		const FVoxelStampDelta& NewStampDelta);

	//~ Begin STableRow Interface
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	//~ End STableRow Interface

private:
	FVoxelStampDelta StampDelta;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class VOXELEDITOR_API SVoxelStampDeltaList : public SCompoundWidget
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ARGUMENT(TVoxelArray<FVoxelStampDelta>, StampDeltas);
	};

	void Construct(const FArguments& Args);

private:
	TArray<TSharedPtr<FVoxelStampDelta>> StampDeltaArray;
	TSharedPtr<SListView<TSharedPtr<FVoxelStampDelta>>> ListView;
};