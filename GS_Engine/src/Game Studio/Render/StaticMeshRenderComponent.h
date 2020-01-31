#pragma once

#include "RenderComponent.h"

#include "Game/StaticMesh.h"

struct StaticMeshRenderComponentCreateInfo : RenderComponentCreateInfo
{
	StaticMesh* StaticMesh = nullptr;
};

class StaticMeshRenderComponent final : public RenderComponent
{
	StaticMesh* staticMesh = nullptr;
	class MaterialRenderResource* renderMaterial = nullptr;
	class MeshRenderResource* renderMesh = nullptr;

	static void CreateInstanceResources(CreateInstanceResourcesInfo& _CIRI);
	static void BuildTypeInstanceSortData(BuildTypeInstanceSortDataInfo& _BTISDI);
	static void BindTypeResources(BindTypeResourcesInfo& _BTRI);
	static void DrawInstance(DrawInstanceInfo& _DII);
	static RenderableInstructions StaticMeshRenderInstructions;
public:
	StaticMeshRenderComponent() = default;

	[[nodiscard]] const char* GetName() const override { return "StaticMeshRenderComponent"; }

	[[nodiscard]] RenderableInstructions* GetRenderableInstructions() const override
	{
		return &StaticMeshRenderInstructions;
	}

	[[nodiscard]] const char* GetRenderableTypeName() const override { return "StaticMesh"; }

	[[nodiscard]] StaticMesh* GetStaticMesh() const { return staticMesh; }
};
