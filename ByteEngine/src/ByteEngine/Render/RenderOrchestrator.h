#pragma once

#include "ByteEngine/Game/System.h"

#include <GTSL/Array.hpp>
#include <GTSL/FlatHashMap.h>
#include <GTSL/FunctionPointer.hpp>

#include "ByteEngine/Id.h"
#include <GTSL/Vector.hpp>

#include "MaterialSystem.h"
#include "RenderSystem.h"
#include "RenderTypes.h"
#include "ByteEngine/Game/Tasks.h"

class RenderOrchestrator;
class RenderState;
class MaterialSystem;
class RenderSystem;
class RenderGroup;
struct TaskInfo;

MAKE_HANDLE(uint32, RenderObject)

class RenderManager : public System
{
public:
	virtual void GetSetupAccesses(GTSL::Array<TaskDependency, 16>& dependencies) = 0;

	struct SetupInfo
	{
		GameInstance* GameInstance;
		RenderSystem* RenderSystem;
		MaterialSystem* MaterialSystem;
		//RenderState* RenderState;
		GTSL::Matrix4 ViewMatrix, ProjectionMatrix;
		RenderOrchestrator* RenderOrchestrator;
	};
	virtual void Setup(const SetupInfo& info) = 0;
};

class StaticMeshRenderManager : public RenderManager
{
	void Initialize(const InitializeInfo& initializeInfo) override;
	void Shutdown(const ShutdownInfo& shutdownInfo) override {}
	
	void GetSetupAccesses(GTSL::Array<TaskDependency, 16>& dependencies) override;

	void Setup(const SetupInfo& info) override;

private:
	MemberHandle matrixUniformBufferMemberHandle;

	SetHandle dataSet;
};

class UIRenderManager : public RenderManager
{
public:
	void Initialize(const InitializeInfo& initializeInfo) override;
	void Shutdown(const ShutdownInfo& shutdownInfo) override {}
	
	void GetSetupAccesses(GTSL::Array<TaskDependency, 16>& dependencies) override;

	void Setup(const SetupInfo& info) override;
	RenderSystem::MeshHandle GetSquareMesh() const { return square; }
	MaterialInstanceHandle GetUIMaterial() const { return uiMaterial; }

private:
	RenderSystem::MeshHandle square;

	MemberHandle matrixUniformBufferMemberHandle, colorHandle;

	SetHandle dataSet;

	uint8 comps = 2;
	MaterialInstanceHandle uiMaterial;
};

class RenderOrchestrator : public System
{
public:
	void Initialize(const InitializeInfo& initializeInfo) override;
	void Shutdown(const ShutdownInfo& shutdownInfo) override;
	
	void Setup(TaskInfo taskInfo);
	void Render(TaskInfo taskInfo);

	void AddRenderManager(GameInstance* gameInstance, const Id renderManager, const SystemHandle systemReference);
	void RemoveRenderManager(GameInstance* gameInstance, const Id renderGroupName, const SystemHandle systemReference);
	
	GTSL::uint8 GetRenderPassColorWriteAttachmentCount(const Id renderPassName)
	{
		auto& renderPass = renderPassesMap[renderPassName()];
		uint8 count = 0;
		for(const auto& e : renderPass.WriteAttachments)
		{
			if (e.Layout == TextureLayout::COLOR_ATTACHMENT || e.Layout == TextureLayout::GENERAL) { ++count; }
		}

		return count;
	}

	enum class TextureComponentType
	{
		FLOAT, INT
	};
	void AddAttachment(Id name, uint8 bitDepth, uint8 componentCount, TextureComponentType compType, TextureType::value_type type, GTSL::RGBA clearColor);

	enum class PassType : uint8
	{
		RASTER, COMPUTE, RAY_TRACING
	};

	struct AttachmentInfo
	{
		Id Name;
		TextureLayout StartState, EndState;
		GAL::RenderTargetLoadOperations Load;
		GAL::RenderTargetStoreOperations Store;
	};
	
	struct PassData
	{
		Id Name;
		
		struct AttachmentReference
		{
			Id Name;
		};
		GTSL::Array<AttachmentReference, 8> ReadAttachments, WriteAttachments;

		PassType PassType;

		Id ResultAttachment;
	};
	void AddPass(RenderSystem* renderSystem, MaterialSystem* materialSystem, GTSL::Range<const PassData*> passesData);

	void OnResize(RenderSystem* renderSystem, MaterialSystem* materialSystem, const GTSL::Extent2D newSize);

	[[nodiscard]] RenderPass getAPIRenderPass(const Id renderPassName) const { return apiRenderPasses[renderPassesMap.At(renderPassName()).APIRenderPass].RenderPass; }

	uint8 GetRenderPassIndex(const Id name) const { return renderPassesMap.At(name()).APIRenderPass; }
	[[nodiscard]] uint8 getAPISubPassIndex(const Id renderPass) const
	{
		uint8 i = 0;
		
		for(auto& e : subPasses[renderPassesMap.At(renderPass()).APIRenderPass]) { if (e.Name == renderPass) { return i; } } 
	}

	[[nodiscard]] FrameBuffer getFrameBuffer(const uint8 rp) const { return apiRenderPasses[rp].FrameBuffer; }
	[[nodiscard]] uint8 getAPIRenderPassesCount() const { return apiRenderPasses.GetLength(); }
	[[nodiscard]] uint8 GetSubPassCount(const uint8 renderPass) const { return subPasses[renderPass].GetLength(); }

	Id GetSubPassName(const uint8 rp, const uint8 sp) { return subPasses[rp][sp].Name; }

	/**
	 * \brief Enables or disables the rendering of a render pass
	 * \param renderPassName Name of the render Pass to toggle
	 * \param enable Whether to enable(true) or disable(false) the render pass
	 */
	void ToggleRenderPass(Id renderPassName, bool enable);

	void AddToRenderPass(Id renderPass, Id renderGroup)
	{
		if(renderPassesMap.Find(renderPass()))
		{
			renderPassesMap.At(renderPass()).RenderGroups.EmplaceBack(renderGroup);
		}
	}

	void AddMesh(const RenderSystem::MeshHandle meshHandle, const MaterialInstanceHandle materialHandle)
	{
		auto result = loadedMaterialInstances.TryGet(materialHandle());

		if (result.State()) [[likely]] {
			result.Get().Meshes.EmplaceBack(meshHandle);
		}
		else
		{
			auto awaitingResult = awaitingMaterialInstances.TryEmplace(materialHandle());

			if (awaitingResult.State()) {
				awaitingResult.Get().Meshes.Initialize(8, GetPersistentAllocator());
			}
			
			awaitingResult.Get().Meshes.EmplaceBack(meshHandle);
		}
	}

	void UpdateMeshIndex(CommandBuffer commandBuffer, RenderSystem* renderSystem, MaterialSystem* materialSystem, RenderSystem::MeshHandle
	                     meshHandle, MaterialInstanceHandle materialHandle);

	RenderObjectHandle GetRenderObjectHandle()
	{
		return RenderObjectHandle(renderObjectId++);
	}

	void ReturnRenderObjectHandle(const RenderObjectHandle renderObjectHandle) {}

private:
	inline static const Id RENDER_TASK_NAME{ "RenderRenderGroups" };
	inline static const Id SETUP_TASK_NAME{ "SetupRenderGroups" };
	inline static const Id CLASS_NAME{ "RenderOrchestrator" };

	GTSL::Atomic<uint32> renderObjectId{ 0 };

	struct RenderState
	{
		GTSL::Vector<RenderObjectHandle, BE::PAR> ObjectsToRender;
	} renderState;
	
	GTSL::Vector<Id, BE::PersistentAllocatorReference> systems;
	GTSL::Vector<GTSL::Array<TaskDependency, 32>, BE::PersistentAllocatorReference> setupSystemsAccesses;
	
	GTSL::FlatHashMap<SystemHandle, BE::PersistentAllocatorReference> renderManagers;

	Id resultAttachment;
	
	GTSL::Array<Id, 8> renderPasses;

	struct MaterialData
	{
		Id MaterialName;
		GTSL::Vector<Id, BE::PAR> MaterialInstances;
	};
	GTSL::FlatHashMap<MaterialData, BE::PAR> readyMaterials;

	struct MaterialInstanceData
	{
		GTSL::Vector<RenderSystem::MeshHandle, BE::PAR> Meshes;
	};
	GTSL::FlatHashMap<MaterialInstanceData, BE::PAR> loadedMaterialInstances, awaitingMaterialInstances;

	struct AttachmentData
	{
		Id Name;
		TextureLayout Layout;
		PipelineStage::value_type ConsumingStages;
	};
	
	struct RenderPassData
	{
		GTSL::Array<Id, 8> RenderGroups;
		PassType PassType;

		SubSetHandle WriteAttachmentsHandle, ReadAttachmentsHandle;
		GTSL::Array<AttachmentData, 8> WriteAttachments, ReadAttachments;

		bool Enabled = false;
		
		uint8 APIRenderPass = 0;
		PipelineStage::value_type PipelineStages;
		SetHandle AttachmentsSetHandle;
	};
	GTSL::FlatHashMap<RenderPassData, BE::PAR> renderPassesMap;
	GTSL::Array<Id, 8> renderPassesNames;

	AccessFlags::value_type accessFlagsFromStageAndAccessType(PipelineStage::value_type, bool writeAccess);
	
	using RenderPassFunctionType = GTSL::FunctionPointer<void(GameInstance*, RenderSystem*, MaterialSystem*, CommandBuffer, Id)>;
	
	GTSL::StaticMap<RenderPassFunctionType, 8> renderPassesFunctions;

	void renderScene(GameInstance*, RenderSystem* renderSystem, MaterialSystem* materialSystem, CommandBuffer commandBuffer, Id rp);
	void renderUI(GameInstance*, RenderSystem* renderSystem, MaterialSystem* materialSystem, CommandBuffer commandBuffer, Id rp);
	void renderRays(GameInstance*, RenderSystem* renderSystem, MaterialSystem* materialSystem, CommandBuffer commandBuffer, Id rp);
	void dispatch(GameInstance* gameInstance, RenderSystem* renderSystem, MaterialSystem* materialSystem,
	              CommandBuffer commandBuffer, Id rp);

	void transitionImages(CommandBuffer commandBuffer, RenderSystem* renderSystem, Id renderPassId);

	void onMaterialLoad(TaskInfo taskInfo, Id materialName);
	void onMaterialInstanceLoad(TaskInfo taskInfo, Id materialName, Id materialInstanceName);
	
	struct APIRenderPassData
	{
		Id Name;
		RenderPass RenderPass;
		FrameBuffer FrameBuffer;
		GTSL::Array<Id, 16> UsedAttachments;
	};
	GTSL::Array<APIRenderPassData, 16> apiRenderPasses;

	struct SubPass
	{
		Id Name;
		uint8 DepthAttachment;
	};
	GTSL::Array<GTSL::Array<SubPass, 16>, 16> subPasses;

	struct Attachment
	{
		TextureFormat Format;
		Texture Texture;
		TextureView TextureView;
		TextureSampler TextureSampler;

		RenderAllocation Allocation;

		Id Name;
		TextureType::value_type Type;
		TextureUses::value_type Uses;
		TextureLayout Layout;
		PipelineStage::value_type ConsumingStages;
		bool WriteAccess = false;
		GTSL::RGBA ClearColor;
	};
	GTSL::StaticMap<Attachment, 32> attachments;

	void updateImage(Attachment& attachment, TextureLayout textureLayout, PipelineStage::value_type stages, bool writeAccess)
	{
		attachment.Layout = textureLayout; attachment.ConsumingStages = stages; attachment.WriteAccess = writeAccess;
	}

public:
	Texture GetAttachmentTexture(const Id attachment) const { return attachments.At(attachment()).Texture; }
	TextureView GetAttachmentTextureView(const Id attachment) const { return attachments.At(attachment()).TextureView; }
	TextureSampler GetAttachmentTextureSampler(const Id attachment) const { return attachments.At(attachment()).TextureSampler; }
};
