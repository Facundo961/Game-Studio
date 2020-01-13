#pragma once

#include "Core.h"

#include "Utility/Extent.h"
#include "Native/VKPipelineLayout.h"
#include "Native/VKGraphicsPipeline.h"
#include "Native/VKComputePipeline.h"
#include "RAPI/RenderMesh.h"
#include "RAPI/GraphicsPipeline.h"
#include "RAPI/ComputePipeline.h"

class VKRenderPass;
class RenderPass;

MAKE_VK_HANDLE(VkPipelineLayout)

class GS_API VulkanGraphicsPipeline final : public GraphicsPipeline
{
	VKGraphicsPipeline Pipeline;

	static VKGraphicsPipelineCreator CreateVk_GraphicsPipelineCreator(VKDevice* _Device, const GraphicsPipelineCreateInfo& _GPCI, VkPipeline _OldPipeline = VK_NULL_HANDLE);
public:
	VulkanGraphicsPipeline(VKDevice* _Device, const GraphicsPipelineCreateInfo& _GPCI);
	~VulkanGraphicsPipeline() = default;

	INLINE const VKGraphicsPipeline& GetVk_GraphicsPipeline() const { return Pipeline; }
};

class GS_API VulkanComputePipeline final : public ComputePipeline
{
	VKComputePipeline ComputePipeline;

public:
	VulkanComputePipeline(VKDevice* _Device);
	~VulkanComputePipeline() = default;

	INLINE const VKComputePipeline& GetVk_ComputePipeline() const { return ComputePipeline; }
};