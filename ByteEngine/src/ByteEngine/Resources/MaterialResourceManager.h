#pragma once

#include <GTSL/Delegate.hpp>
#include <GTSL/File.h>
#include <GTSL/FlatHashMap.h>
#include "ResourceManager.h"

namespace GAL {
	enum class BindingType : unsigned char;
	enum class ShaderType : unsigned char;
	enum class ShaderDataType : unsigned char;
}

class MaterialResourceManager final : public ResourceManager
{
public:
	MaterialResourceManager();
	~MaterialResourceManager();
	
	struct MaterialInfo
	{
		uint32 MaterialOffset = 0;
		GTSL::Id64 RenderGroup;
		GTSL::Array<uint32, 12> ShaderSizes;
		GTSL::Array<uint8, 20> VertexElements;
		GTSL::Array<GTSL::Array<uint8, 12>, 12> BindingSets;
		GTSL::Array<uint8, 12> ShaderTypes;
		friend void Insert(const MaterialInfo& materialInfo, GTSL::Buffer& buffer);
		friend void Extract(MaterialInfo& materialInfo, GTSL::Buffer& buffer);
	};

	struct MaterialCreateInfo
	{
		GTSL::StaticString<128> ShaderName;
		GTSL::StaticString<128> RenderGroup;
		GTSL::Ranger<const GAL::ShaderDataType> VertexFormat;
		GTSL::Ranger<const GTSL::Ranger<const GAL::BindingType>> BindingSets;
		GTSL::Ranger<const GAL::ShaderType> ShaderTypes;
	};
	void CreateMaterial(const MaterialCreateInfo& materialCreateInfo);

	void GetMaterialSize(GTSL::Id64 name, uint32& size);
	
	struct OnMaterialLoadInfo : OnResourceLoad
	{
		GTSL::Id64 RenderGroup;
		GTSL::Array<GAL::ShaderDataType, 20> VertexElements;
		GTSL::Array<GTSL::Array<GAL::BindingType, 12>, 12> BindingSets;
		GTSL::Array<GAL::ShaderType, 12> ShaderTypes;
		GTSL::Array<uint32, 20> ShaderSizes;
	};
	
	struct MaterialLoadInfo : ResourceLoadInfo
	{
		GTSL::Delegate<void(TaskInfo, OnMaterialLoadInfo)> OnMaterialLoad;
	};
	void LoadMaterial(const MaterialLoadInfo& loadInfo);
	
private:
	GTSL::File package, index;
	GTSL::FlatHashMap<MaterialInfo, BE::PersistentAllocatorReference> materialInfos;
	GTSL::ReadWriteMutex mutex;
};
