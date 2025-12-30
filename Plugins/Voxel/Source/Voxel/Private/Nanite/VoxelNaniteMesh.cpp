// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nanite/VoxelNaniteMesh.h"
#include "MegaMaterial/VoxelMegaMaterialProxy.h"
#include "VoxelMesh.h"
#include "VoxelSubsystem.h"
#include "VoxelBufferPool.h"
#include "VoxelNaniteBuilder.h"
#include "VoxelMetadataMaterialType.h"

#include "Engine/StaticMesh.h"
#include "Rendering/NaniteResources.h"

DEFINE_VOXEL_COUNTER(STAT_VoxelNumNaniteMeshes);
DEFINE_VOXEL_COUNTER(STAT_VoxelNumNanitePages);
DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelNaniteMemory);
DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelNaniteMesh);

TVoxelFuture<TSharedPtr<FVoxelNaniteMesh>> FVoxelNaniteMesh::Create(
	FVoxelSubsystem& Subsystem,
	const TSharedRef<FVoxelMesh>& Mesh,
	const FVoxelChunkNeighborInfo& NeighborInfo)
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedRef<FVoxelNaniteMesh> Result = MakeShareable(new FVoxelNaniteMesh(Mesh, NeighborInfo));
	Subsystem.AddGCObject(Result);
	return Result->Initialize(Subsystem);
}

FVoxelNaniteMesh::~FVoxelNaniteMesh()
{
	Voxel::GameTask([WeakStaticMesh = WeakStaticMesh]
	{
		VOXEL_FUNCTION_COUNTER();

		UStaticMesh* StaticMeshToRelease = WeakStaticMesh.Resolve();
		if (!StaticMeshToRelease)
		{
			return;
		}

		StaticMeshToRelease->ReleaseResources();

		// Wait for render thread before doing SetRenderData(nullptr) otherwise FStreamingManager::Remove crashes
		Voxel::RenderTask([=]
		{
			Voxel::GameTask([=]
			{
				UStaticMesh* StaticMeshToDestroy = WeakStaticMesh.Resolve();
				if (!StaticMeshToDestroy)
				{
					return;
				}

				StaticMeshToDestroy->SetRenderData(nullptr);
				StaticMeshToDestroy->MarkAsGarbage();
			});
		});
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNaniteMesh::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(StaticMesh);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelNaniteMesh::FVoxelNaniteMesh(
	const TSharedRef<FVoxelMesh>& Mesh,
	const FVoxelChunkNeighborInfo& NeighborInfo)
	: Mesh(Mesh)
	, NeighborInfo(NeighborInfo)
{
}

TVoxelFuture<TSharedPtr<FVoxelNaniteMesh>> FVoxelNaniteMesh::Initialize(const FVoxelSubsystem& Subsystem)
{
	VOXEL_FUNCTION_COUNTER();
	ensure(Mesh->Indices.Num() > 0);

	TVoxelArray<int32> VertexOffsets;
	const TSharedRef<TUniquePtr<FStaticMeshRenderData>> RenderDataRef = INLINE_LAMBDA
	{
		const TVoxelArray<FVector3f> Vertices = Mesh->GetDisplacedVertices(NeighborInfo);

		int32 Scale = 1;
		if (Mesh->ChunkLOD <= Subsystem.GetConfig().NaniteMaxTessellationLOD)
		{
			Scale *= (1 << Mesh->ChunkLOD) * Subsystem.GetConfig().VoxelSize;
		}

		TVoxelArray<FVector3f> Positions;
		TVoxelArray<FVoxelOctahedron> Normals;

		Positions.Reserve(Mesh->Indices.Num());
		Normals.Reserve(Mesh->Indices.Num());

		ensure(Mesh->Indices.Num() % 3 == 0);
		for (int32 TriangleIndex = 0; TriangleIndex < Mesh->Indices.Num() / 3; TriangleIndex++)
		{
			const int32 IndexA = Mesh->Indices[3 * TriangleIndex + 0];
			const int32 IndexB = Mesh->Indices[3 * TriangleIndex + 1];
			const int32 IndexC = Mesh->Indices[3 * TriangleIndex + 2];

			// Scale positions here to avoid different displacement scale on different LODs
			Positions.Add_EnsureNoGrow(Vertices[IndexA] * Scale);
			Positions.Add_EnsureNoGrow(Vertices[IndexB] * Scale);
			Positions.Add_EnsureNoGrow(Vertices[IndexC] * Scale);

			Normals.Add_EnsureNoGrow(Mesh->Normals[IndexA]);
			Normals.Add_EnsureNoGrow(Mesh->Normals[IndexB]);
			Normals.Add_EnsureNoGrow(Mesh->Normals[IndexC]);
		}

		FVoxelNaniteBuilder NaniteBuilder;
		NaniteBuilder.Mesh.Positions = Positions;
		NaniteBuilder.Mesh.Normals = Normals;

		NaniteBuilder.PositionPrecision = Subsystem.GetConfig().NanitePositionPrecision;

		TUniquePtr<FStaticMeshRenderData> RenderData = NaniteBuilder.CreateRenderData(VertexOffsets);
		{
			FStaticMeshSectionArray& Sections = RenderData->LODResources[0].Sections;
			Sections.Reset();

			// Actual material we use for rendering
			Sections.Emplace_GetRef().MaterialIndex = 0;
			// World grid material
			Sections.Emplace_GetRef().MaterialIndex = 1;

			for (int32 Index = 0; Index < Mesh->UsedSurfaceTypes.Num(); Index++)
			{
				Sections.Emplace_GetRef().MaterialIndex = 2 + Index;
			}
		}
		return MakeSharedCopy(MoveTemp(RenderData));
	};

	if (!ensure(*RenderDataRef))
	{
		return nullptr;
	}

	{
		VOXEL_SCOPE_COUNTER("Attributes");

		const FVoxelMegaMaterialProxy& MegaMaterialProxy = *Subsystem.GetConfig().MegaMaterialProxy;

		TVoxelMap<int32, TSharedRef<FVoxelBufferRef>> IndexToBufferRef;

		{
			TVoxelArray<FVoxelRenderMaterial> RenderMaterials = MegaMaterialProxy.GetRenderMaterials(Mesh->SurfaceTypes);

			const FVoxelBufferUpload Upload = MegaMaterialProxy.GetMaterialBufferPool().Upload_AnyThread(MoveTemp(RenderMaterials));
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

			const TSharedPtr<FVoxelTextureBufferPool> BufferPool = MegaMaterialProxy.GetMetadataBufferPool(Metadata);
			if (!ensure(BufferPool))
			{
				continue;
			}

			const int32 MetadataIndex = MegaMaterialProxy.GetMetadataIndexToMetadata().Find(Metadata);
			if (!ensure(MetadataIndex != -1))
			{
				continue;
			}

			const FVoxelBufferUpload Upload = BufferPool->Upload_AnyThread(MoveTemp(Bytes));
			// Upload.Future will be waited on implicitly by the task context

			IndexToBufferRef.Add_EnsureNew(MetadataIndex + 1, Upload.BufferRef);
		}

		const TConstVoxelArrayView<FVoxelMetadataRef> Metadatas = MegaMaterialProxy.GetMetadataIndexToMetadata();

		const int32 NumAttributes = 1 + Metadatas.Num();

		TVoxelArray<int32> AttributeIndices;
		FVoxelUtilities::SetNumFast(AttributeIndices, Mesh->Indices.Num() * NumAttributes);
		FVoxelUtilities::SetAll(AttributeIndices, -1);

		for (const auto& It : IndexToBufferRef)
		{
			BufferRefs.Add(It.Value);

			const int32 AttributeIndex = It.Key;
			const int64 BufferIndex = It.Value->GetIndex();
			checkVoxelSlow(BufferIndex + Mesh->Vertices.Num() <= MAX_int32);

			for (int32 Index = 0; Index < Mesh->Indices.Num(); Index++)
			{
				AttributeIndices[Index * NumAttributes + AttributeIndex] = int32(BufferIndex) + Mesh->Indices[Index];
			}
		}

		const FVoxelBufferUpload Upload = MegaMaterialProxy.GetAttributeIndicesBufferPool().Upload_AnyThread(MoveTemp(AttributeIndices));
		// Upload.Future will be waited on implicitly by the task context

		AttributeIndicesBufferRef = Upload.BufferRef;
	};

	const int32 NumPages = (**RenderDataRef).NaniteResourcesPtr->NumRootPages;
	ensure(VertexOffsets.Num() == NumPages);

	NumNaniteMeshes = 1;
	NumNanitePages = NumPages;
	NaniteMemory = NumPages * NANITE_ROOT_PAGE_GPU_SIZE;

	return Voxel::GameTask(MakeStrongPtrLambda(this, [=, this]() -> TVoxelFuture<TSharedPtr<FVoxelNaniteMesh>>
	{
		StaticMesh = FVoxelNaniteBuilder::CreateStaticMesh(MoveTemp(*RenderDataRef));
		WeakStaticMesh = StaticMesh;

		if (!ensure(StaticMesh))
		{
			return nullptr;
		}

		// Add fake materials so that NumMaterials returns what we want in GetStreamingRenderAssetInfo
		{
			TArray<FStaticMaterial> StaticMaterials;
			// One for the actual material we use for rendering
			// One for the world grid material
			StaticMaterials.SetNum(2 + Mesh->UsedSurfaceTypes.Num());

			for (FStaticMaterial& StaticMaterial : StaticMaterials)
			{
				// Fix ensure in UStaticMesh::GetUVChannelData
				// Assume a default texel density of 100 for texture streaming
				StaticMaterial.UVChannelData = FMeshUVChannelInfo(100.f);
			}

			StaticMesh->SetStaticMaterials(StaticMaterials);
		}

		return Voxel::RenderTask(MakeStrongPtrLambda(this, [=, this]() -> TSharedPtr<FVoxelNaniteMesh>
		{
			const FStaticMeshRenderData* MeshRenderData = StaticMesh->GetRenderData();
			if (!ensure(MeshRenderData) ||
				!ensure(MeshRenderData->NaniteResourcesPtr))
			{
				return nullptr;
			}

			const int32 RootPageIndex = MeshRenderData->NaniteResourcesPtr->RootPageIndex;
			if (!ensure(RootPageIndex != -1))
			{
				return nullptr;
			}

			for (int32 Index = 0; Index < NumPages; Index++)
			{
				Pages.Add(FPage
				{
					RootPageIndex + Index,
					VertexOffsets[Index]
				});
			}

			return AsShared();
		}));
	}));
}