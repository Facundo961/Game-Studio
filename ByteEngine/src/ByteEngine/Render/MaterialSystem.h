#pragma once

#include <GAL/RenderCore.h>
#include <GTSL/Array.hpp>
#include <GTSL/Buffer.h>
#include <GTSL/SparseVector.hpp>
#include <GTSL/Vector.hpp>
#include <GTSL/StaticMap.hpp>
#include <GTSL/PagedVector.h>
#include <GTSL/Tree.hpp>

#include "RenderTypes.h"
#include "ByteEngine/Game/System.h"
#include "ByteEngine/Resources/MaterialResourceManager.h"
#include "ByteEngine/Resources/TextureResourceManager.h"

#include "ByteEngine/Handle.hpp"

class TextureResourceManager;
struct TaskInfo;
class RenderSystem;

struct MaterialHandle
{
	Id MaterialType;
	uint32 MaterialInstance = 0;
	uint32 Element = 0;
};

struct MemberDescription
{
	uint8 SetBufferDataIndex, StructSize, DataType;
};

MAKE_HANDLE(Id, Set)
MAKE_HANDLE(MemberDescription, Member)

class MaterialSystem : public System
{	
public:
	MaterialSystem() : System("MaterialSystem")
	{}

	struct Member
	{
		enum class DataType : uint8
		{
			FLOAT32, INT32, UINT32, MATRIX4, FVEC4, FVEC2
		};

		uint32 Count = 1;
		DataType Type;
	};
	
	void Initialize(const InitializeInfo& initializeInfo) override;
	void Shutdown(const ShutdownInfo& shutdownInfo) override;

	template<typename T>
	T* GetMemberPointer(MemberHandle member, uint64 index);

	template<>
	GTSL::Matrix4* GetMemberPointer(MemberHandle member, uint64 index)
	{
		GTSL_ASSERT(Member::DataType(member().DataType) == Member::DataType::MATRIX4, "Type mismatch");
		return getSetMemberPointer<GTSL::Matrix4>(member(), index, frame);
	}

	template<>
	int32* GetMemberPointer(MemberHandle member, uint64 index)
	{
		GTSL_ASSERT(Member::DataType(member().DataType) == Member::DataType::INT32, "Type mismatch");
		return getSetMemberPointer<int32>(member(), index, frame);
	}

	template<>
	uint32* GetMemberPointer(MemberHandle member, uint64 index)
	{
		GTSL_ASSERT(Member::DataType(member().DataType) == Member::DataType::UINT32, "Type mismatch");
		return getSetMemberPointer<uint32>(member(), index, frame);
	}

	Pipeline GET_PIPELINE(MaterialHandle materialHandle);
	void BIND_SET(RenderSystem* renderSystem, CommandBuffer commandBuffer, SetHandle set, uint32 index);
	PipelineLayout GET_PIPELINE_LAYOUT(SetHandle handle) { return setNodes.At((Id)handle)->Data.PipelineLayout; }

	struct MemberInfo : Member
	{
		MemberHandle* Handle;
	};

	enum class Frequency : uint8
	{
		PER_INSTANCE
	};
	
	struct StructInfo
	{
		Frequency Frequency;
		GTSL::Range<MemberInfo*> Members;
		uint64* Handle;
	};
	
	struct SetInfo
	{
		GTSL::Range<StructInfo*> Structs;
	};
	SetHandle AddSet(RenderSystem* renderSystem, Id setName, Id parent, const SetInfo& setInfo);
	SetHandle AddSet(RenderSystem* renderSystem, Id setName, Id parent, const SetInfo& setInfo, PipelineLayout::PushConstant* pushConstant);

	
	void AddObjects(RenderSystem* renderSystem, SetHandle set, uint32 count);

	struct BindingsSetData
	{
		BindingsSetLayout BindingsSetLayout;
		BindingsSet BindingsSets[MAX_CONCURRENT_FRAMES];
		uint32 DataSize = 0;
	};
	
	struct CreateMaterialInfo
	{
		Id MaterialName;
		MaterialResourceManager* MaterialResourceManager = nullptr;
		GameInstance* GameInstance = nullptr;
		RenderSystem* RenderSystem = nullptr;
		TextureResourceManager* TextureResourceManager;
	};
	[[nodiscard]] MaterialHandle CreateMaterial(const CreateMaterialInfo& info);

	void SetDynamicMaterialParameter(const MaterialHandle material, GAL::ShaderDataType type, Id parameterName, void* data);
	void SetMaterialParameter(const MaterialHandle material, GAL::ShaderDataType type, Id parameterName, void* data);

	[[nodiscard]] auto GetMaterialHandles() const { return readyMaterialHandles.GetRange(); }
	
private:
	void updateDescriptors(TaskInfo taskInfo);
	void updateCounter(TaskInfo taskInfo);

	template<typename T>
	T* getSetMemberPointer(MemberDescription member, uint64 index, uint8 frameToUpdate)
	{
		auto& setBufferData = setsBufferData[member.SetBufferDataIndex];
		auto memberSize = setBufferData.MemberSize;
		return reinterpret_cast<T*>(static_cast<byte*>(setBufferData.Allocations[frameToUpdate].Data) + (index * memberSize) + member.StructSize);
	}
	
	uint32 matNum = 0;
	
	struct MaterialData
	{
		struct MaterialInstanceData
		{
			
		};
		
		GTSL::KeepVector<MaterialInstanceData, BE::PAR> MaterialInstances;

		SetHandle Set;
		RasterizationPipeline Pipeline;
		MemberHandle TextureRefHandle[8];
		uint64 TextureRefsTableHandle;
	};
	GTSL::KeepVector<MaterialData, BE::PAR> materials;

	struct PendingMaterialData : MaterialData
	{
		PendingMaterialData(uint32 targetValue, MaterialData&& materialData) : MaterialData(materialData), Target(targetValue) {}
		
		uint32 Counter = 0, Target = 0;
		MaterialHandle Material;
	};
	GTSL::KeepVector<PendingMaterialData, BE::PAR> pendingMaterials;
	GTSL::FlatHashMap<uint32, BE::PAR> readyMaterialsMap;
	GTSL::Vector<MaterialHandle, BE::PAR> readyMaterialHandles;
	
	struct CreateTextureInfo
	{
		Id TextureName;
		GameInstance* GameInstance = nullptr;
		RenderSystem* RenderSystem = nullptr;
		TextureResourceManager* TextureResourceManager = nullptr;
		MaterialHandle MaterialHandle;
	};
	ComponentReference createTexture(const CreateTextureInfo& createTextureInfo);

	struct DescriptorsUpdate
	{
		DescriptorsUpdate() = default;

		void Initialize(const BE::PAR& allocator)
		{
			setsToUpdate.Initialize(4, allocator);
			PerSetToUpdateBufferBindingsUpdate.Initialize(4, allocator);
			PerSetToUpdateTextureBindingsUpdate.Initialize(4, allocator);
		}

		[[nodiscard]] uint32 AddSetToUpdate(uint32 set, const BE::PAR& allocator)
		{
			const auto handle = setsToUpdate.EmplaceBack(set);
			PerSetToUpdateBufferBindingsUpdate.EmplaceBack(4, allocator);
			PerSetToUpdateTextureBindingsUpdate.EmplaceBack(4, allocator);
			return handle;
		}

		void AddBufferUpdate(uint32 set, uint32 firstArrayElement, BindingsSet::BufferBindingsUpdateInfo update)
		{
			PerSetToUpdateBufferBindingsUpdate[set].EmplaceAt(firstArrayElement, update);
		}

		void AddTextureUpdate(uint32 set, uint32 firstArrayElement, BindingsSet::TextureBindingsUpdateInfo update)
		{
			PerSetToUpdateTextureBindingsUpdate[set].EmplaceAt(firstArrayElement, update);
		}
		
		void Reset()
		{
			setsToUpdate.ResizeDown(0);
			PerSetToUpdateBufferBindingsUpdate.ResizeDown(0);
			PerSetToUpdateTextureBindingsUpdate.ResizeDown(0);
		}
		
		GTSL::Vector<uint32, BE::PAR> setsToUpdate;

		GTSL::Vector<GTSL::SparseVector<BindingsSet::BufferBindingsUpdateInfo, BE::PAR>, BE::PAR> PerSetToUpdateBufferBindingsUpdate;
		GTSL::Vector<GTSL::SparseVector<BindingsSet::TextureBindingsUpdateInfo, BE::PAR>, BE::PAR> PerSetToUpdateTextureBindingsUpdate;
	};
	GTSL::Array<DescriptorsUpdate, MAX_CONCURRENT_FRAMES> descriptorsUpdates;
	
	struct RenderGroupData
	{
		uint32 SetReference;
	};
	GTSL::FlatHashMap<RenderGroupData, BE::PAR> renderGroupsData;
	
	struct SetData
	{
		Id Name;
		void* Parent;
		uint32 Level = 0;
		PipelineLayout PipelineLayout;
		BindingsSetLayout BindingsSetLayout;
		BindingsPool BindingsPool;
		uint32 SetBufferData = 0xFFFFFFFF;
	};
	
	GTSL::Tree<SetData, BE::PAR> setsTree;
	GTSL::FlatHashMap<decltype(setsTree)::Node*, BE::PAR> setNodes;

	struct Struct
	{
		enum class Frequency : uint8
		{
			PER_INSTANCE
		} Frequency;
		
		GTSL::Array<Member, 8> Members;
	};
	
	struct SetBufferData
	{
		/**
		 * \brief Size (in bytes) of the structure this set has. Right now is only one "Member" but could be several.
		 */
		uint32 MemberSize = 0;
		HostRenderAllocation Allocations[MAX_CONCURRENT_FRAMES];
		Buffer Buffers[MAX_CONCURRENT_FRAMES];

		uint32 UsedInstances = 0, AllocatedInstances = 0;
		
		GTSL::Array<uint32, 8> AllocatedStructsPerInstance;
		GTSL::Array<uint16, 8> StructsSizes;
		GTSL::Array<Struct, 8> Structs;

		BindingsSet BindingsSet[MAX_CONCURRENT_FRAMES];
	};
	GTSL::KeepVector<SetBufferData, BE::PAR> setsBufferData;

	GTSL::PagedVector<uint32, BE::PAR> queuedBufferUpdates;
	
	struct TextureLoadInfo
	{
		TextureLoadInfo(uint32 component, Buffer buffer, RenderSystem* renderSystem, HostRenderAllocation renderAllocation) : Component(component), Buffer(buffer), RenderSystem(renderSystem), RenderAllocation(renderAllocation)
		{
		}

		uint32 Component;
		Buffer Buffer;
		RenderSystem* RenderSystem;
		HostRenderAllocation RenderAllocation;
	};
	void onTextureLoad(TaskInfo taskInfo, TextureResourceManager::OnTextureLoadInfo loadInfo);
	
	void onTextureProcessed(TaskInfo taskInfo, TextureResourceManager::OnTextureLoadInfo loadInfo);
	
	struct TextureComponent
	{
		Texture Texture;
		TextureView TextureView;
		TextureSampler TextureSampler;
		RenderAllocation Allocation;
	};
	GTSL::KeepVector<TextureComponent, BE::PersistentAllocatorReference> textures;
	GTSL::FlatHashMap<uint32, BE::PersistentAllocatorReference> texturesRefTable;

	MAKE_HANDLE(uint32, PendingMaterial)
	
	GTSL::Vector<uint32, BE::PAR> latestLoadedTextures;
	GTSL::KeepVector<GTSL::Vector<PendingMaterialHandle, BE::PAR>, BE::PersistentAllocatorReference> pendingMaterialsPerTexture;
	
	void addPendingMaterialToTexture(uint32 texture, PendingMaterialHandle material)
	{
		pendingMaterialsPerTexture[texture].EmplaceBack(material);
	}
	
	struct MaterialLoadInfo
	{
		MaterialLoadInfo(RenderSystem* renderSystem, GTSL::Buffer&& buffer, uint32 index, TextureResourceManager* tRM) : RenderSystem(renderSystem), Buffer(MoveRef(buffer)), Component(index), TextureResourceManager(tRM)
		{

		}

		RenderSystem* RenderSystem = nullptr;
		GTSL::Buffer Buffer;
		uint32 Component;
		TextureResourceManager* TextureResourceManager;
	};
	void onMaterialLoaded(TaskInfo taskInfo, MaterialResourceManager::OnMaterialLoadInfo onMaterialLoadInfo);
	
	template<typename C, typename C2>
	void genShaderStages(RenderDevice* renderDevice, C& container, C2& shaderInfos, const MaterialResourceManager::OnMaterialLoadInfo& onMaterialLoadInfo)
	{
		uint32 offset = 0;
		
		for (uint32 i = 0; i < onMaterialLoadInfo.ShaderTypes.GetLength(); ++i)
		{
			Shader::CreateInfo create_info;
			create_info.RenderDevice = renderDevice;
			create_info.ShaderData = GTSL::Range<const byte*>(onMaterialLoadInfo.ShaderSizes[i], onMaterialLoadInfo.DataBuffer.begin() + offset);
			container.EmplaceBack(create_info);
			offset += onMaterialLoadInfo.ShaderSizes[i];
		}

		for (uint32 i = 0; i < container.GetLength(); ++i)
		{
			shaderInfos.PushBack({ ConvertShaderType(onMaterialLoadInfo.ShaderTypes[i]), &container[i] });
		}
	}

	void createBuffers(RenderSystem* renderSystem, const uint32 bufferSet);

	uint16 minUniformBufferOffset = 0;
	
	uint8 frame;
	const uint8 queuedFrames = 2;

	SetHandle makeSetEx(RenderSystem* renderSystem, Id setName, Id parent, GTSL::Range<BindingsSetLayout::BindingDescriptor*> bindingDescriptors, PipelineLayout::PushConstant* pushConstant);
	
	void resizeSet(RenderSystem* renderSystem, uint32 set);
};
