#pragma once

#include "Core.h"

#include "Vulkan\Vulkan.h"

#include "RenderContext.h"
#include "Shader.h"
#include "Buffer.h"
#include "CommandBuffer.h"
#include "Pipelines.h"
#include "RenderPass.h"

enum class RAPI : uint8
{
	NONE, VULKAN
};

GS_CLASS Renderer
{
	static RAPI RenderAPI;
	static Renderer* RendererInstance;
	
	static Renderer* CreateRenderer();
public:
	static INLINE RAPI GetRenderAPI() { return RenderAPI; }
	static INLINE Renderer* GetRenderer() { return RendererInstance; }

	virtual RenderContext* CreateRenderContext(const RenderContextCreateInfo& _RCI) = 0;
	virtual Shader* CreateShader(const ShaderCreateInfo& _SI) = 0;
	virtual Buffer* CreateBuffer(const BufferCreateInfo& _BCI) = 0;
	virtual CommandBuffer* CreateCommandBuffer(const CommandBufferCreateInfo& _CBCI) = 0;
	virtual GraphicsPipeline* CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& _GPCI) = 0;
	virtual ComputePipeline* CreateComputePipeline(const ComputePipelineCreateInfo& _CPCI) = 0;
	virtual RenderPass* CreateRenderPass(const RenderPassCreateInfo& _RPCI) = 0;
	virtual Framebuffer* CreateFramebuffer(const FramebufferCreateInfo& _FCI) = 0;
};

