// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelRuntimePinValue.h"
#include "VoxelBuffer.h"
#include "VoxelObjectPinType.h"
#include "VoxelPinValueOps.h"
#include "Buffer/VoxelDoubleBuffers.h"

void FVoxelRuntimePinValue::InitializeImpl()
{
	if (Type.IsBuffer())
	{
		if (Type.IsBufferArray())
		{
			TSharedRef<FVoxelBuffer> Buffer = FVoxelBuffer::MakeEmpty(Type.GetInnerType());
			SharedStructType = Buffer->GetStruct();
			SharedStruct = MakeSharedVoidRef(MoveTemp(Buffer));
		}
		else
		{
			TSharedRef<FVoxelBuffer> Buffer = FVoxelBuffer::MakeDefault(Type.GetInnerType());
			SharedStructType = Buffer->GetStruct();
			SharedStruct = MakeSharedVoidRef(MoveTemp(Buffer));
		}
	}
	else if (Type.IsStruct())
	{
		SharedStructType = Type.GetStruct();
		SharedStruct = MakeSharedStruct(SharedStructType);
	}
}

FVoxelRuntimePinValue FVoxelRuntimePinValue::MakeStruct(const FConstVoxelStructView Struct)
{
	FVoxelRuntimePinValue Result;
	Result.Type = FVoxelPinType::MakeStruct(Struct.GetScriptStruct());
	Result.SharedStructType = Struct.GetScriptStruct();
	Result.SharedStruct = Struct.MakeSharedCopy();
	return Result;
}

FVoxelRuntimePinValue FVoxelRuntimePinValue::Make(FVoxelBuffer&& Buffer)
{
	const TSharedRef<FVoxelBuffer> NewBuffer = FVoxelBuffer::MakeEmpty(Buffer.GetInnerType());
	NewBuffer->MoveFrom(MoveTemp(Buffer));
	return	Make(NewBuffer);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelRuntimePinValue::IsValidValue_Slow() const
{
	if (!Type.IsValid_Fast())
	{
		return false;
	}

	if (!Type.CanBeCastedTo<FVoxelBuffer>())
	{
		return true;
	}

	return Get<FVoxelBuffer>().IsValid_Slow();
}

FString FVoxelRuntimePinValue::ToDebugString(const bool bFullValue) const
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(IsValid()))
	{
		return {};
	}

	if (Type.IsBuffer())
	{
		return {};
	}

	switch (Type.GetInternalType())
	{
	default:
	{
		ensure(false);
		return {};
	}
	case EVoxelPinInternalType::Bool:
	{
		return LexToString(Get<bool>());
	}
	case EVoxelPinInternalType::Float:
	{
		return bFullValue ? LexToString(Get<float>()) : FVoxelUtilities::NumberToString(Get<float>());
	}
	case EVoxelPinInternalType::Double:
	{
		return bFullValue ? LexToString(Get<double>()) : FVoxelUtilities::NumberToString(Get<double>());
	}
	case EVoxelPinInternalType::UInt16:
	{
		return bFullValue ? LexToString(Get<uint16>()) : FVoxelUtilities::NumberToString(Get<uint16>());
	}
	case EVoxelPinInternalType::Int32:
	{
		return bFullValue ? LexToString(Get<int32>()) : FVoxelUtilities::NumberToString(Get<int32>());
	}
	case EVoxelPinInternalType::Int64:
	{
		return bFullValue ? LexToString(Get<int64>()) : FVoxelUtilities::NumberToString(Get<int64>());
	}
	case EVoxelPinInternalType::Name:
	{
		return Get<FName>().ToString();
	}
	case EVoxelPinInternalType::Byte:
	{
		if (const UEnum* Enum = Type.GetEnum())
		{
			return Enum->GetNameStringByValue(Byte);
		}
		return LexToString(Get<uint8>());
	}
	case EVoxelPinInternalType::Class:
	{
		return FSoftObjectPath(Class).ToString();
	}
	case EVoxelPinInternalType::Struct:
	{
		if (Type.Is<FVector2D>())
		{
			if (bFullValue)
			{
				return Get<FVector2D>().ToString();
			}

			const FVector2D& Value = Get<FVector2D>();
			return
				"X=" + FVoxelUtilities::NumberToString(Value.X) + " " +
				"Y=" + FVoxelUtilities::NumberToString(Value.Y);
		}
		else if (Type.Is<FVector>())
		{
			if (bFullValue)
			{
				return Get<FVector>().ToString();
			}

			const FVector& Value = Get<FVector>();
			return
				"X=" + FVoxelUtilities::NumberToString(Value.X) + " " +
				"Y=" + FVoxelUtilities::NumberToString(Value.Y) + " " +
				"Z=" + FVoxelUtilities::NumberToString(Value.Z);
		}
		else if (Type.Is<FLinearColor>())
		{
			const FLinearColor& Value = Get<FLinearColor>();
			if (bFullValue)
			{
				return
					"R=" + LexToSanitizedString(Value.R) + " " +
					"G=" + LexToSanitizedString(Value.G) + " " +
					"B=" + LexToSanitizedString(Value.B) + " " +
					"A=" + LexToSanitizedString(Value.A);
			}

			return
				"R=" + FVoxelUtilities::NumberToString(Value.R) + " " +
				"G=" + FVoxelUtilities::NumberToString(Value.G) + " " +
				"B=" + FVoxelUtilities::NumberToString(Value.B) + " " +
				"A=" + FVoxelUtilities::NumberToString(Value.A);
		}
		else if (Type.Is<FVoxelDoubleVector2D>())
		{
			if (bFullValue)
			{
				return Get<FVoxelDoubleVector2D>().ToString();
			}

			const FVoxelDoubleVector2D& Value = Get<FVoxelDoubleVector2D>();
			return
				"X=" + FVoxelUtilities::NumberToString(Value.X) + " " +
				"Y=" + FVoxelUtilities::NumberToString(Value.Y);
		}
		else if (Type.Is<FVoxelDoubleVector>())
		{
			if (bFullValue)
			{
				return Get<FVoxelDoubleVector>().ToString();
			}

			const FVoxelDoubleVector& Value = Get<FVoxelDoubleVector>();
			return
				"X=" + FVoxelUtilities::NumberToString(Value.X) + " " +
				"Y=" + FVoxelUtilities::NumberToString(Value.Y) + " " +
				"Z=" + FVoxelUtilities::NumberToString(Value.Z);
		}
		else if (Type.Is<FVoxelDoubleLinearColor>())
		{
			const FVoxelDoubleLinearColor& Value = Get<FVoxelDoubleLinearColor>();
			if (bFullValue)
			{
				return
					"R=" + LexToSanitizedString(Value.R) + " " +
					"G=" + LexToSanitizedString(Value.G) + " " +
					"B=" + LexToSanitizedString(Value.B) + " " +
					"A=" + LexToSanitizedString(Value.A);
			}

			return
				"R=" + FVoxelUtilities::NumberToString(Value.R) + " " +
				"G=" + FVoxelUtilities::NumberToString(Value.G) + " " +
				"B=" + FVoxelUtilities::NumberToString(Value.B) + " " +
				"A=" + FVoxelUtilities::NumberToString(Value.A);
		}
		else if (Type.Is<FIntPoint>())
		{
			if (bFullValue)
			{
				return Get<FIntPoint>().ToString();
			}

			const FIntPoint& Value = Get<FIntPoint>();
			return
				"X=" + FVoxelUtilities::NumberToString(Value.X) + " " +
				"Y=" + FVoxelUtilities::NumberToString(Value.Y);
		}
		else if (Type.Is<FIntVector>())
		{
			if (bFullValue)
			{
				return Get<FIntVector>().ToString();
			}

			const FIntVector& Value = Get<FIntVector>();
			return
				"X=" + FVoxelUtilities::NumberToString(Value.X) + " " +
				"Y=" + FVoxelUtilities::NumberToString(Value.Y) + " " +
				"Z=" + FVoxelUtilities::NumberToString(Value.Z);
		}
		else if (Type.Is<FIntVector4>())
		{
			const FIntVector4& Value = Get<FIntVector4>();
			if (bFullValue)
			{
				return Value.ToString();
			}

			return
				"X=" + FVoxelUtilities::NumberToString(Value.X) + " " +
				"Y=" + FVoxelUtilities::NumberToString(Value.Y) + " " +
				"Z=" + FVoxelUtilities::NumberToString(Value.Z) + " " +
				"W=" + FVoxelUtilities::NumberToString(Value.W);
		}
		else if (Type.Is<FInt64Point>())
		{
			if (bFullValue)
			{
				return Get<FInt64Point>().ToString();
			}

			const FInt64Point& Value = Get<FInt64Point>();
			return
				"X=" + FVoxelUtilities::NumberToString(Value.X) + " " +
				"Y=" + FVoxelUtilities::NumberToString(Value.Y);
		}
		else if (Type.Is<FInt64Vector>())
		{
			if (bFullValue)
			{
				return Get<FInt64Vector>().ToString();
			}

			const FInt64Vector& Value = Get<FInt64Vector>();
			return
				"X=" + FVoxelUtilities::NumberToString(Value.X) + " " +
				"Y=" + FVoxelUtilities::NumberToString(Value.Y) + " " +
				"Z=" + FVoxelUtilities::NumberToString(Value.Z);
		}
		else if (Type.Is<FInt64Vector4>())
		{
			const FInt64Vector4& Value = Get<FInt64Vector4>();
			if (bFullValue)
			{
				return Value.ToString();
			}

			return
				"X=" + FVoxelUtilities::NumberToString(Value.X) + " " +
				"Y=" + FVoxelUtilities::NumberToString(Value.Y) + " " +
				"Z=" + FVoxelUtilities::NumberToString(Value.Z) + " " +
				"W=" + FVoxelUtilities::NumberToString(Value.W);
		}
		else if (Type.Is<FRotator>())
		{
			return Get<FRotator>().ToString();
		}
		else if (Type.Is<FQuat>())
		{
			return Get<FQuat>().Rotator().ToString();
		}
		else if (Type.Is<FColor>())
		{
			return Get<FColor>().ToString();
		}
		else if (const TSharedPtr<FVoxelPinValueOps> Ops = FVoxelPinValueOps::Find(Type, EVoxelPinValueOpsUsage::ToDebugString))
		{
			return Ops->ToDebugString(*this);
		}
		else if (Type.GetExposedType().IsObject())
		{
			const UObject* Object = Get<FVoxelObjectPinType>().GetObject(GetStructView());
			if (!Object)
			{
				return "None";
			}

			return Object->GetName();
		}
		else
		{
			return FVoxelUtilities::PropertyToText_Direct(
				*FVoxelUtilities::MakeStructProperty(Type.GetStruct()),
				GetStructView().GetStructMemory(),
				nullptr);
		}
	}
	}
}