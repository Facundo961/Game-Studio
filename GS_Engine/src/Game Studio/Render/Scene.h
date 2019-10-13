#pragma once

#include "Core.h"
#include "Camera.h"
#include "Containers/FVector.hpp"
#include "Math/Matrix4.h"
#include "Game/SubWorlds.h"
#include "RenderComponent.h"

#include "ScreenQuad.h"
#include "RAPI/Window.h"
#include "RAPI/Framebuffer.h"
#include "RAPI/GraphicsPipeline.h"
#include "RAPI/UniformBuffer.h"
#include "RAPI/RenderContext.h"
#include "RAPI/RenderPass.h"

#include "RenderResourcesManager.h"
#include <map>
#include "RenderableInstructions.h"

class StaticMeshResource;
class RenderProxy;
class PointLightRenderProxy;

//Stores all the data necessary for the RAPI to work. It's the RAPIs representation of the game world.
class GS_API Scene : public SubWorld
{
public:
	Scene();
	virtual ~Scene();

	void OnUpdate() override;	

	//Returns a pointer to the active camera.
	[[nodiscard]] Camera* GetActiveCamera() const { return ActiveCamera; }
	[[nodiscard]] const Matrix4& GetViewMatrix() const { return ViewMatrix; }
	[[nodiscard]] const Matrix4& GetProjectionMatrix() const { return ProjectionMatrix; }
	[[nodiscard]] const Matrix4& GetVPMatrix() const { return ViewProjectionMatrix; }

	//Sets the active camera as the NewCamera.
	void SetCamera(Camera * NewCamera) { ActiveCamera = NewCamera; }

	template<class T>
	T* CreateRenderComponent(WorldObject* _Owner) const
	{
		RenderComponent* NRC = new T();
		this->RegisterRenderComponent(NRC);
		return static_cast<T*>(NRC);
	}

	[[nodiscard]] const char* GetName() const override { return "Scene"; }

	void DrawMesh(const DrawInfo& _DI);
protected:
	GS_DEBUG_ONLY(uint32 DrawCalls = 0)

	mutable RenderResourcesManager ResourcesManager;

	mutable std::map<Id::HashType, RenderableInstructions> RenderableInstructionsMap;

	//Scene elements
	mutable FVector<RenderComponent*> RenderComponents;

	//Pointer to the active camera.
	Camera* ActiveCamera = nullptr;

	//Render elements
	Window* Win = nullptr;
	FVector<Framebuffer*> Framebuffers;
	ScreenQuad MyQuad = {};
	RenderContext* RC = nullptr;
	RenderPass* RP = nullptr;
	UniformBuffer* UB = nullptr;
	UniformLayout* UL = nullptr;

	Matrix4 ViewMatrix;
	Matrix4 ProjectionMatrix;
	Matrix4 ViewProjectionMatrix;

	void UpdateMatrices();

	void RegisterRenderComponent(RenderComponent* _RC) const;
	void RenderRenderables();

	//Returns a symmetric perspective frustum.
	static Matrix4 BuildPerspectiveMatrix(const float FOV, const float AspectRatio, const float Near, const float Far);

	//Returns a perspective frustum.
	static Matrix4 BuildPerspectiveFrustum(const float Right, const float Left, const float Top, const float Bottom, const float Near, const float Far);
};