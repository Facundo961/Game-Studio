#pragma once

#include <GAL/RenderCore.h>
#include <GTSL/Array.hpp>
#include <GTSL/Buffer.h>
#include <GTSL/Vector.hpp>
#include <GTSL/StaticMap.hpp>

#include "RenderTypes.h"
#include "ByteEngine/Game/System.h"
#include "ByteEngine/Resources/MaterialResourceManager.h"

struct TaskInfo;
class RenderSystem;

class MaterialSystem : public System
{
public:
	MaterialSystem() : System("MaterialSystem"), materialNames(16, GetPersistentAllocator())
	{}

	void Initialize(const InitializeInfo& initializeInfo) override;
	void Shutdown(const ShutdownInfo& shutdownInfo) override;

	void SetGlobalState(GameInstance* gameInstance, const GTSL::Array<GTSL::Array<BindingType, 6>, 6>& globalState);

	struct AddRenderGroupInfo
	{
		Id Name;
		GTSL::Array<GTSL::Array<BindingType, 6>, 6> Bindings;
	};
	void AddRenderGroup(GameInstance* gameInstance, const AddRenderGroupInfo& addRenderGroupInfo);
	
	struct MaterialInstance
	{
		BindingsSetLayout BindingsSetLayout;
		RasterizationPipeline Pipeline;
		BindingsPool BindingsPool;
		PipelineLayout PipelineLayout;
		GTSL::Array<BindingsSet, MAX_CONCURRENT_FRAMES> BindingsSets;

		Buffer Buffer;
		void* Data;
		RenderAllocation Allocation;

		uint32 DataSize = 0;
		
		GTSL::StaticMap<GTSL::Pair<Id, uint32>, 16> Parameters;

		MaterialInstance() = default;
	};

	struct RenderGroupData
	{
		BindingsSetLayout BindingsSetLayout;
		BindingsPool BindingsPool;
		PipelineLayout PipelineLayout;
		GTSL::Array<BindingsSet, MAX_CONCURRENT_FRAMES> BindingsSets;

		uint32 DataSize;
		
		Buffer Buffer;
		void* Data;
		RenderAllocation Allocation;

		GTSL::FlatHashMap<MaterialInstance, BE::PersistentAllocatorReference> Instances;

		RenderGroupData() = default;
	};

	GTSL::FlatHashMap<RenderGroupData, BE::PersistentAllocatorReference>& GetRenderGroups() { return renderGroups; }
	
	struct CreateMaterialInfo
	{
		Id MaterialName;
		MaterialResourceManager* MaterialResourceManager = nullptr;
		GameInstance* GameInstance = nullptr;
		RenderSystem* RenderSystem = nullptr;
	};
	ComponentReference CreateMaterial(const CreateMaterialInfo& info);

	void SetMaterialParameter(const ComponentReference material, GAL::ShaderDataType type, Id parameterName,
	                          void* data);

	void SetMaterialTexture(const ComponentReference material, Id parameterName, const uint8 n, TextureView* image, TextureSampler* sampler);

	GTSL::Array<BindingsSetLayout, 6> globalBindingsSetLayout;
	GTSL::Array<BindingsSet, MAX_CONCURRENT_FRAMES> globalBindingsSets;
	BindingsPool globalBindingsPool;
	PipelineLayout globalPipelineLayout;

	bool IsMaterialReady(const uint64 renderGroup, const uint64 material) { return isRenderGroupReady.At(renderGroup) && isMaterialReady.At(material); }
private:
	void updateDescriptors(TaskInfo taskInfo);
	void updateCounter(TaskInfo taskInfo);

	GTSL::KeepVector<GTSL::Pair<Id, Id>, BE::PersistentAllocatorReference> materialNames;
	GTSL::FlatHashMap<uint8, BE::PersistentAllocatorReference> isRenderGroupReady;
	GTSL::FlatHashMap<uint8, BE::PersistentAllocatorReference> isMaterialReady;

	struct BindingsUpdateData
	{
		struct Updates
		{			
			Vector<BindingsSet::TextureBindingsUpdateInfo> TextureBindingDescriptorsUpdates;
			Vector<BindingsSet::BufferBindingsUpdateInfo> BufferBindingDescriptorsUpdates;
			Vector<BindingType> BufferBindingTypes;
			Id Name;
			Id Name2;
		};

		Updates Global;
		GTSL::FlatHashMap<Updates, BE::PersistentAllocatorReference> RenderGroups;
		GTSL::FlatHashMap<Updates, BE::PersistentAllocatorReference> Materials;

		BindingsUpdateData() = default;
		void Initialize(const uint32 num, const BE::PersistentAllocatorReference& allocator)
		{
			Global.BufferBindingDescriptorsUpdates.Initialize(8, allocator);
			Global.TextureBindingDescriptorsUpdates.Initialize(8, allocator);
			Global.BufferBindingTypes.Initialize(8, allocator);
			RenderGroups.Initialize(4, allocator); Materials.Initialize(4, allocator);
		}
	};
	GTSL::Array<BindingsUpdateData, MAX_CONCURRENT_FRAMES> perFrameBindingsUpdateData;
	
	ComponentReference component = 0;

	GTSL::FlatHashMap<RenderGroupData, BE::PersistentAllocatorReference> renderGroups;
	
	struct MaterialLoadInfo
	{
		MaterialLoadInfo(RenderSystem* renderSystem, GTSL::Buffer&& buffer, uint32 index) : RenderSystem(renderSystem), Buffer(MoveRef(buffer)), Component(index)
		{

		}

		RenderSystem* RenderSystem = nullptr;
		GTSL::Buffer Buffer;
		uint32 Component;
	};
	void onMaterialLoaded(TaskInfo taskInfo, MaterialResourceManager::OnMaterialLoadInfo onMaterialLoadInfo);

	uint16 minUniformBufferOffset = 0;

	uint8 frame;
};
