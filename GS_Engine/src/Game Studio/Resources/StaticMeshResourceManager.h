#pragma once

#include "SubResourceManager.h"
#include <unordered_map>
#include "Vertex.h"

struct StaticMeshResourceData final : ResourceData
{
	//Pointer to Vertex Array.
	Vertex* VertexArray = nullptr;
	//Pointer to index array.
	uint16* IndexArray = nullptr;

	//Vertex Count.
	uint16 VertexCount = 0;
	//Index Count.
	uint16 IndexCount = 0;

	~StaticMeshResourceData()
	{
		delete[] VertexArray;
		delete[] IndexArray;
	}
};

class StaticMeshResourceManager final : public SubResourceManager
{
public:
	const char* GetResourceExtension() override { return "obj"; }
	bool LoadResource(const LoadResourceInfo& loadResourceInfo, OnResourceLoadInfo& onResourceLoadInfo) override;
	void LoadFallback(const LoadResourceInfo& loadResourceInfo, OnResourceLoadInfo& onResourceLoadInfo) override;
	ResourceData* GetResource(const Id& name) override;
	void ReleaseResource(const Id& resourceName) override;
	[[nodiscard]] Id GetResourceType() const override { return "Static Mesh"; }
	
private:
	std::unordered_map<Id, StaticMeshResourceData> resources;
};
