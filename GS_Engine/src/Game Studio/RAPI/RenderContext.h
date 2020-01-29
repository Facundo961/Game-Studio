#pragma once

#include "Core.h"
#include "Utility/Extent.h"
#include "Image.h"
#include "Containers/FVector.hpp"

class BindingLayout;
class Window;
class RenderMesh;
class VertexBuffer;
class IndexBuffer;
class RenderPass;
class GraphicsPipeline;
class ComputePipeline;
class Framebuffer;

struct GS_API DrawInfo
{
	uint16 IndexCount = 0;
	uint16 InstanceCount = 1;
};

struct GS_API RenderPassBeginInfo
{
	RenderPass* RenderPass = nullptr;
	Framebuffer* Framebuffer = nullptr;
};

struct GS_API PushConstantsInfo
{
	class Pipeline* Pipeline = nullptr;
	uint32 Offset = 0;
	uint32 Size = 0;
	void* Data = nullptr;
};

struct GS_API RenderContextCreateInfo
{
	Window* Window = nullptr;
};

struct ResizeInfo
{
	Extent2D NewWindowSize;
};

struct CopyToSwapchainInfo
{
	Image* Image = nullptr;
};

struct BindBindingsSet
{
	/**
	 * \brief What index in the bindings set to use.
	 */
	uint8 BindingSetIndex = 0;
	Pair<uint8, class BindingsSet**> BindingsSets;
	class Pipeline* Pipeline = nullptr;
};

class GS_API RenderContext
{
protected:
	uint8 CurrentImage = 0;
	uint8 MAX_FRAMES_IN_FLIGHT = 0;

public:
	virtual ~RenderContext()
	{
	};

	virtual void OnResize(const ResizeInfo& _RI) = 0;

	//Starts recording of commands.
	virtual void BeginRecording() = 0;
	//Ends recording of commands.
	virtual void EndRecording() = 0;

	virtual void AcquireNextImage() = 0;

	//Sends all commands to the GPU.
	virtual void Flush() = 0;

	//Swaps buffers and sends new image to the screen.
	virtual void Present() = 0;

	// COMMANDS

	//  BIND COMMANDS
	//    BIND BUFFER COMMANDS


	//Adds a BindMesh command to the command queue.
	virtual void BindMesh(RenderMesh* _Mesh) = 0;

	//    BIND PIPELINE COMMANDS

	//Adds a BindBindingsSet to the command queue.
	virtual void BindBindingsSet(const BindBindingsSet& bindBindingsSet) = 0;
	virtual void UpdatePushConstant(const PushConstantsInfo& _PCI) = 0;
	//Adds a BindGraphicsPipeline command to the command queue.
	virtual void BindGraphicsPipeline(GraphicsPipeline* _GP) = 0;
	//Adds a BindComputePipeline to the command queue.
	virtual void BindComputePipeline(ComputePipeline* _CP) = 0;


	//  DRAW COMMANDS

	//Adds a DrawIndexed command to the command queue.
	virtual void DrawIndexed(const DrawInfo& _DrawInfo) = 0;

	//  COMPUTE COMMANDS

	//Adds a Dispatch command to the command queue.
	virtual void Dispatch(const Extent3D& _WorkGroups) = 0;

	//  RENDER PASS COMMANDS

	//Adds a BeginRenderPass command to the command queue.
	virtual void BeginRenderPass(const RenderPassBeginInfo& _RPBI) = 0;
	//Adds a AdvanceSubPass command to the command buffer.
	virtual void AdvanceSubPass() = 0;
	//Adds a EndRenderPass command to the command queue.
	virtual void EndRenderPass(RenderPass* _RP) = 0;

	virtual void CopyToSwapchain(const CopyToSwapchainInfo& copyToSwapchainInfo) = 0;

	[[nodiscard]] virtual FVector<Image*> GetSwapchainImages() const = 0;

	[[nodiscard]] uint8 GetCurrentImage() const { return CurrentImage; }
	[[nodiscard]] uint8 GetMaxFramesInFlight() const { return MAX_FRAMES_IN_FLIGHT; }
};
