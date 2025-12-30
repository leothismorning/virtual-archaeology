// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelGraphQuery.h"
#include "VoxelPointId.h"
#include "VoxelPointSet.generated.h"

struct FVoxelGraphNodeRef;

struct VOXELPCG_API FVoxelPointAttributes
{
	static const FName Id;
	static const FName Mesh;
	static const FName Position;
	static const FName Rotation;
	static const FName Scale;
	static const FName Density;
	static const FName BoundsMin;
	static const FName BoundsMax;
	static const FName Color;
	static const FName Steepness;

	static FName MakeActor(const FName Name)
	{
		return TEXTVIEW("Actor.") + Name;
	}
	static FName MakeParent(const FName Name)
	{
		return TEXTVIEW("Parent.") + Name;
	}
	static FName MakeCustomData(const int32 Index)
	{
		return FName(STATIC_FNAME("CustomData"), Index + 1);
	}
};

USTRUCT()
struct VOXELPCG_API FVoxelPointSet
	: public FVoxelVirtualStruct
	, public TSharedFromThis<FVoxelPointSet>
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	FORCEINLINE int32 Num() const
	{
		return PrivateNum;
	}
	FORCEINLINE bool Contains(const FName Name) const
	{
		return NameToAttribute.Contains(Name);
	}

	FORCEINLINE TSharedPtr<const FVoxelBuffer> Find(const FName Name) const
	{
		return NameToAttribute.FindRef(Name);
	}
	template<typename T>
	FORCEINLINE const T* FindPtr(const FName Name) const
	{
		const TSharedPtr<const FVoxelBuffer> Buffer = Find(Name);
		if (!Buffer ||
			!ensureVoxelSlow(Buffer->IsA<T>()))
		{
			return nullptr;
		}

		return &Buffer->AsChecked<T>();
	};

	FORCEINLINE const TVoxelMap<FName, TSharedPtr<const FVoxelBuffer>>& GetAttributes() const
	{
		return NameToAttribute;
	}

public:
	void SetNum(int32 NewNum);
	void Add(FName Name, const TSharedRef<const FVoxelBuffer>& Buffer);
	FVoxelGraphQuery MakeQuery(FVoxelGraphQuery Query) const;
	bool CheckNum(const FVoxelNode* Node, int32 BufferNum) const;
	TSharedRef<FVoxelPointSet> Gather(TConstVoxelArrayView<int32> Indices) const;
	int64 GetAllocatedSize() const;

public:
	static TSharedRef<const FVoxelPointSet> Merge(TVoxelArray<TSharedRef<const FVoxelPointSet>> PointSets);

public:
	template<typename T>
	requires std::derived_from<T, FVoxelBuffer>
	void Add(const FName Name, T&& Buffer)
	{
		this->Add(Name, MakeSharedCopy(MoveTemp(Buffer)));
	}

private:
	int32 PrivateNum = 0;
	TVoxelMap<FName, TSharedPtr<const FVoxelBuffer>> NameToAttribute;
};

namespace FVoxelGraphParameters
{
	struct VOXELPCG_API FPointSet : FBufferParameter
	{
		TSharedPtr<const FVoxelPointSet> Value;

		void Split(
			const FVoxelBufferSplitter& Splitter,
			TConstVoxelArrayView<FPointSet*> OutResult) const;
	};
}

#define FindVoxelPointSetAttribute(PointSet, Name, Type, VariableName) \
	if ((PointSet).Num() > 0) \
	{ \
		const TSharedPtr<const FVoxelBuffer> Buffer = (PointSet).Find(Name); \
		if (!Buffer) \
		{ \
			VOXEL_MESSAGE(Error, "{0}: Missing attribute {1}", this, Name); \
			return; \
		} \
		if (!Buffer->IsA<Type>()) \
		{ \
			VOXEL_MESSAGE(Error, "{0}: attribute {1} has type {2}, but type {3} was expectd", \
				this, \
				Name, \
				Buffer->GetBufferType().ToString(), \
				FVoxelPinType::Make<Type>().ToString()); \
			return; \
		} \
	} \
	const Type EMPTY_ ## VariableName; \
	const Type& VariableName = (PointSet).Num() == 0 ? EMPTY_ ## VariableName : (PointSet).Find(Name)->AsChecked<Type>();

#define FindVoxelPointSetOptionalAttribute(PointSet, Name, Type, VariableName, Default) \
	TSharedPtr<const FVoxelBuffer> VOXEL_APPEND_LINE(_Buffer); \
	{ \
		VOXEL_APPEND_LINE(_Buffer) = (PointSet).Find(Name); \
		if (VOXEL_APPEND_LINE(_Buffer) && \
			!VOXEL_APPEND_LINE(_Buffer)->IsA<Type>()) \
		{ \
			VOXEL_MESSAGE(Error, "{0}: attribute {1} has type {2}, but type {3} was expectd", \
				this, \
				Name, \
				VOXEL_APPEND_LINE(_Buffer)->GetBufferType().ToString(), \
				FVoxelPinType::Make<Type>().ToString()); \
			VOXEL_APPEND_LINE(_Buffer) = nullptr; \
		} \
	} \
	const Type DEFAULT_ ## VariableName = Type(Default); \
	const Type& VariableName = \
		VOXEL_APPEND_LINE(_Buffer) \
		? static_cast<const Type&>(*VOXEL_APPEND_LINE(_Buffer)) \
		: DEFAULT_ ## VariableName;
