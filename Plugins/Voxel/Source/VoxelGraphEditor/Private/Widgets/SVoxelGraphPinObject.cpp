// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "SVoxelGraphPinObject.h"
#include "VoxelPinType.h"
#include "VoxelObjectPinType.h"
#include "SLevelOfDetailBranchNode.h"
#include "SVoxelGraphObjectSelector.h"

void SVoxelGraphPinObject::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	VOXEL_FUNCTION_COUNTER();

	SGraphPinObject::Construct(SGraphPinObject::FArguments(), InGraphPinObj);
	GetLabelAndValue()->SetWrapSize(300.f);

	const auto GetFullWidgetSize = [this]
	{
		static FVector2D DefaultSize(20.f, 57.f);

		const TSharedPtr<SHorizontalBox> Box = FullPinHorizontalRowWidget.Pin();
		if (!Box)
		{
			return DefaultSize;
		}

		const FVector2D Size = Box->GetDesiredSize();
		if (Size == FVector2D::ZeroVector)
		{
			return DefaultSize;
		}

		return Size;
	};

	SBorder::Construct(SBorder::FArguments()
		.BorderImage(this, &SVoxelGraphPinObject::GetPinBorder)
		.BorderBackgroundColor(this, &SVoxelGraphPinObject::GetHighlightColor)
		.OnMouseButtonDown(this, &SVoxelGraphPinObject::OnPinNameMouseDown)
		[
			SNew(SBorder)
			.BorderImage(CachedImg_Pin_DiffOutline)
			.BorderBackgroundColor(this, &SVoxelGraphPinObject::GetPinDiffColor)
			[
				SNew(SLevelOfDetailBranchNode)
				.UseLowDetailSlot(this, &SVoxelGraphPinObject::UseLowDetailPinNames)
				.LowDetail()
				[
					SNew(SBox)
					.WidthOverride_Lambda([=]
					{
						return GetFullWidgetSize().X;
					})
					.HeightOverride_Lambda([=]
					{
						return GetFullWidgetSize().Y;
					})
					.HAlign(GetDirection() == EGPD_Input ? HAlign_Left : HAlign_Right)
					.VAlign(VAlign_Center)
					[
						PinImage.ToSharedRef()
					]
				]
				.HighDetail()
				[
					FullPinHorizontalRowWidget.Pin().ToSharedRef()
				]
			]
		]
	);
}

TSharedRef<SWidget>	SVoxelGraphPinObject::GetDefaultValueWidget()
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(GraphPinObj))
	{
		return SNullWidget::NullWidget;
	}

	const FVoxelPinType PinType = FVoxelPinType(GraphPinObj->PinType).GetInnerType();

	if (!ensureVoxelSlow(PinType.IsValid()) ||
		!ensureVoxelSlow(PinType.IsStruct()))
	{
		// Will happen if we remove a FVoxelObjectPinType
		return
			SNew(SBox)
			.MaxDesiredWidth(200.f)
			[
				SNew(SVoxelGraphObjectSelector)
				.Visibility(this, &SGraphPin::GetDefaultValueVisibility)
				.IsEnabled(false)
				.AllowedClasses({ UObject::StaticClass() })
				.ThumbnailPool(FVoxelEditorUtilities::GetThumbnailPool())
				.ObjectPath_Lambda([this]
				{
					return GetAssetData(true).GetSoftObjectPath().ToString();
				})
				.OnObjectChanged(this, &SVoxelGraphPinObject::OnAssetSelectedFromPicker)
			];
	}

	const TSharedPtr<const FVoxelObjectPinType> ObjectPinType = FVoxelObjectPinType::StructToPinType().FindRef(PinType.GetStruct());
	if (!ensure(ObjectPinType))
	{
		return SNullWidget::NullWidget;
	}

	return
		SNew(SBox)
		.MaxDesiredWidth(200.f)
		[
			SNew(SVoxelGraphObjectSelector)
			.Visibility(this, &SGraphPin::GetDefaultValueVisibility)
			.IsEnabled(this, &SGraphPin::IsEditingEnabled)
			.AllowedClasses(ObjectPinType->GetAllowedClasses())
			.ThumbnailPool(FVoxelEditorUtilities::GetThumbnailPool())
			.ObjectPath_Lambda([this]
			{
				return GetAssetData(true).GetSoftObjectPath().ToString();
			})
			.OnObjectChanged(this, &SVoxelGraphPinObject::OnAssetSelectedFromPicker)
		];
}

void SVoxelGraphPinObject::OnAssetSelectedFromPicker(const FAssetData& AssetData)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(!GraphPinObj->IsPendingKill()) ||
		AssetData == GetAssetData(true))
	{
		return;
	}

	const FVoxelTransaction Transaction(GraphPinObj, "Change Object Pin Value");

	GraphPinObj->GetSchema()->TrySetDefaultObject(*GraphPinObj, AssetData.GetAsset());
}