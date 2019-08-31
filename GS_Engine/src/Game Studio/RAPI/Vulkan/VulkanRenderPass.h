#pragma once

#include "RAPI/RenderPass.h"

#include "Native/VKRenderPass.h"

GS_CLASS VulkanRenderPass final : public RenderPass
{
	VKRenderPass RenderPass;

	static VKRenderPassCreator CreateInfo(VKDevice* _Device, const RenderPassDescriptor& _RPD);
public:
	VulkanRenderPass(VKDevice* _Device, const RenderPassDescriptor & _RPD);
	~VulkanRenderPass() = default;

	INLINE const VKRenderPass& GetVk_RenderPass() const { return RenderPass; }
};