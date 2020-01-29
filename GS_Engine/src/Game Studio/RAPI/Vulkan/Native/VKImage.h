#pragma once

#include "Core.h"
#include "RAPI/Vulkan/VulkanBase.h"

MAKE_VK_HANDLE(VkImage)

struct VkMemoryRequirements;

struct VkImageCreateInfo;

struct GS_API VKImageCreator final : VKObjectCreator<VkImage>
{
	VKImageCreator(VKDevice* _Device, const VkImageCreateInfo* _VkICI);
};


class GS_API VKImage final : public VKObject<VkImage>
{
public:
	VKImage(const VKImageCreator& _VKIC) : VKObject<VkImage>(_VKIC)
	{
	}

	~VKImage();

	[[nodiscard]] VkMemoryRequirements GetMemoryRequirements() const;
};
