#pragma once

#include "SubResourceManager.h"
#include <unordered_map>
#include <GTSL/Extent.h>
#include "ResourceData.h"

struct TextureResourceData final : ResourceData
{
	byte* ImageData = nullptr;
	size_t ImageDataSize = 0;
	Extent2D TextureDimensions;
	//GAL::ImageFormat TextureFormat;
	
	~TextureResourceData();
};

class TextureResourceManager final : public SubResourceManager
{
public:
	inline static constexpr GTSL::Id64 type{ "Texture" };
	
	TextureResourceManager() : SubResourceManager("Texture")
	{
	}
	
	TextureResourceData* GetResource(const GTSL::Id64& name)
	{
		ReadLock<ReadWriteMutex> lock(resourceMapMutex);
		return &resources[name];
	}

	TextureResourceData* TryGetResource(const GTSL::String& name);
	
	void ReleaseResource(const GTSL::Id64& resourceName)
	{
		resourceMapMutex.WriteLock();
		if (resources[resourceName].DecrementReferences() == 0) { resources.erase(resourceName); }
		resourceMapMutex.WriteUnlock();
	}
	
private:
	std::unordered_map<GTSL::Id64::HashType, TextureResourceData> resources;
	
};
