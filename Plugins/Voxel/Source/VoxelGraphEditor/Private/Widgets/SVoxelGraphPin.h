// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "KismetPins/SGraphPinBool.h"
#include "KismetPins/SGraphPinClass.h"
#include "KismetPins/SGraphPinColor.h"
#include "KismetPins/SGraphPinString.h"
#include "KismetPins/SGraphPinVector.h"
#include "KismetPins/SGraphPinInteger.h"
#include "KismetPins/SGraphPinVector2D.h"

#define VOXEL_PIN_MAKE_EDITABLE_LABEL(Widget, Parent) \
	virtual TSharedRef<SWidget> GetLabelWidget(const FName& InPinLabelStyle) override \
	{ \
		return \
			SVoxelGraphPin::InitializeEditableLabel( \
				Widget::SharedThis(this), \
				Parent::GetLabelWidget(InPinLabelStyle), \
				TAttribute<EVisibility>::CreateSP(this, &Widget::GetPinLabelVisibility), \
				TAttribute<FText>::CreateSP(this, &Widget::GetPinLabel), \
				TAttribute<FSlateColor>::CreateSP(this, &Widget::GetPinTextColor)); \
	}

class SVoxelGraphPin : public SGraphPin
{
protected:
	VOXEL_PIN_MAKE_EDITABLE_LABEL(SVoxelGraphPin, SGraphPin)

public:
	static TSharedRef<SWidget> InitializeEditableLabel(
		const TSharedPtr<SGraphPin>& PinWidget,
		const TSharedRef<SWidget>& DefaultWidget,
		const TAttribute<EVisibility>& PinLabelVisibilityAttribute,
		const TAttribute<FText>& PinLabelAttribute,
		const TAttribute<FSlateColor>& ColorAndOpacityAttribute);
};

class SVoxelGraphPinBool : public SGraphPinBool
{
protected:
	VOXEL_PIN_MAKE_EDITABLE_LABEL(SVoxelGraphPinBool, SGraphPinBool)
};

class SVoxelGraphPinInteger : public SGraphPinInteger
{
protected:
	VOXEL_PIN_MAKE_EDITABLE_LABEL(SVoxelGraphPinInteger, SGraphPinInteger)
};

class SVoxelGraphPinString : public SGraphPinString
{
protected:
	VOXEL_PIN_MAKE_EDITABLE_LABEL(SVoxelGraphPinString, SGraphPinString)
};

template <typename NumericType>
class SVoxelGraphPinVector2D : public SGraphPinVector2D<NumericType>
{
protected:
	VOXEL_PIN_MAKE_EDITABLE_LABEL(SVoxelGraphPinVector2D<NumericType>, SGraphPinVector2D<NumericType>)
};

template <typename NumericType>
class SVoxelGraphPinVector : public SGraphPinVector<NumericType>
{
protected:
	VOXEL_PIN_MAKE_EDITABLE_LABEL(SVoxelGraphPinVector<NumericType>, SGraphPinVector<NumericType>)
};

class SVoxelGraphPinColor : public SGraphPinColor
{
protected:
	VOXEL_PIN_MAKE_EDITABLE_LABEL(SVoxelGraphPinColor, SGraphPinColor)
};

class SVoxelGraphPinClass : public SGraphPinClass
{
protected:
	VOXEL_PIN_MAKE_EDITABLE_LABEL(SVoxelGraphPinClass, SGraphPinClass)
};

template <typename NumericType>
class SVoxelGraphPinNum : public SGraphPinNum<NumericType>
{
protected:
	VOXEL_PIN_MAKE_EDITABLE_LABEL(SVoxelGraphPinNum<NumericType>, SGraphPinNum<NumericType>)
};