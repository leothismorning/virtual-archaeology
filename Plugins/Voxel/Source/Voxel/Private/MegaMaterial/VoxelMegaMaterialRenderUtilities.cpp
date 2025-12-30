// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "MegaMaterial/VoxelMegaMaterialRenderUtilities.h"
#include "MegaMaterial/VoxelMegaMaterialProxy.h"
#include "VoxelMesh.h"
#include "VoxelBufferPool.h"
#include "VoxelMetadataMaterialType.h"

TSharedRef<const FVoxelMegaMaterialRenderData> FVoxelMegaMaterialRenderUtilities::BuildRenderData(
	const TSharedRef<FVoxelMegaMaterialProxy>& MegaMaterialProxy,
	const TSharedRef<const FVoxelMesh>& Mesh)
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelMap<int32, TSharedRef<FVoxelBufferRef>> IndexToBufferRef;

	{
		TVoxelArray<FVoxelRenderMaterial> RenderMaterials = MegaMaterialProxy->GetRenderMaterials(Mesh->SurfaceTypes);

		const FVoxelBufferUpload Upload = MegaMaterialProxy->GetMaterialBufferPool().Upload_AnyThread(MoveTemp(RenderMaterials));
		// Upload.Future will be waited on implicitly by the task context

		IndexToBufferRef.Add_EnsureNew(0, Upload.BufferRef);
	}

	for (const auto& It : Mesh->MetadataToBuffer)
	{
		const FVoxelMetadataRef Metadata = It.Key;
		VOXEL_SCOPE_COUNTER_FNAME(Metadata.GetFName());

		const TVoxelOptional<EVoxelMetadataMaterialType> MaterialType = Metadata.GetMaterialType();
		if (!ensure(MaterialType))
		{
			continue;
		}

		TVoxelArray<uint8> Bytes;
		FVoxelUtilities::SetNumFast(Bytes, Mesh->Vertices.Num() * FVoxelMetadataMaterialType::GetTypeSize(*MaterialType));

		Metadata.WriteMaterialData(*It.Value, Bytes);

		if (*MaterialType == EVoxelMetadataMaterialType::Float3 ||
			*MaterialType == EVoxelMetadataMaterialType::Int3)
		{
			VOXEL_SCOPE_COUNTER("Add component");

			TVoxelArray<uint8> NewBytes;
			FVoxelUtilities::SetNumFast(NewBytes, Mesh->Vertices.Num() * sizeof(FVector4f));

			for (int32 Index = 0; Index < Mesh->Vertices.Num(); Index++)
			{
				NewBytes.View<FVector4f>()[Index] = FVector4f(Bytes.View<FVector3f>()[Index], 0.f);
			}

			Bytes = MoveTemp(NewBytes);
		}

		const TSharedPtr<FVoxelTextureBufferPool> BufferPool = MegaMaterialProxy->GetMetadataBufferPool(Metadata);
		if (!ensure(BufferPool))
		{
			continue;
		}

		const int32 MetadataIndex = MegaMaterialProxy->GetMetadataIndexToMetadata().Find(Metadata);
		if (!ensure(MetadataIndex != -1))
		{
			continue;
		}

		const FVoxelBufferUpload Upload = BufferPool->Upload_AnyThread(MoveTemp(Bytes));
		// Upload.Future will be waited on implicitly by the task context

		IndexToBufferRef.Add_EnsureNew(MetadataIndex + 1, Upload.BufferRef);
	}

	const TConstVoxelArrayView<FVoxelMetadataRef> Metadatas = MegaMaterialProxy->GetMetadataIndexToMetadata();

	const int32 NumAttributes = 1 + Metadatas.Num();

	TVoxelArray<int32> AttributeIndices;
	FVoxelUtilities::SetNumFast(AttributeIndices, Mesh->Vertices.Num() * NumAttributes);
	FVoxelUtilities::SetAll(AttributeIndices, -1);

	const TSharedRef<FVoxelMegaMaterialRenderData> RenderData = MakeShared<FVoxelMegaMaterialRenderData>();
	RenderData->MegaMaterialProxy = MegaMaterialProxy;

	for (const auto& It : IndexToBufferRef)
	{
		RenderData->BufferRefs.Add(It.Value);

		const int32 AttributeIndex = It.Key;
		const int64 BufferIndex = It.Value->GetIndex();
		checkVoxelSlow(BufferIndex + Mesh->Vertices.Num() <= MAX_int32);

		for (int32 Index = 0; Index < Mesh->Vertices.Num(); Index++)
		{
			AttributeIndices[Index * NumAttributes + AttributeIndex] = int32(BufferIndex) + Index;
		}
	}

	const FVoxelBufferUpload Upload = MegaMaterialProxy->GetAttributeIndicesBufferPool().Upload_AnyThread(MoveTemp(AttributeIndices));
	// Upload.Future will be waited on implicitly by the task context

	RenderData->BufferRefs.Add(Upload.BufferRef);
	RenderData->AttributesIndicesOffset = Upload.BufferRef->GetIndex();
	return RenderData;
}