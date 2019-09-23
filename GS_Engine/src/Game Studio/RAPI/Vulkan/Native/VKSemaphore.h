#pragma once

#include "Core.h"

#include "RAPI/Vulkan/VulkanBase.h"

MAKE_VK_HANDLE(VkSemaphore)

struct VkSemaphoreCreateInfo;

struct GS_API VKSemaphoreCreator final : VKObjectCreator<VkSemaphore>
{
	VKSemaphoreCreator(VKDevice* _Device, const VkSemaphoreCreateInfo * _VkSCI);
};

class GS_API VKSemaphore final : public VKObject<VkSemaphore>
{
public:
	VKSemaphore(const VKSemaphoreCreator& _VKSC) : VKObject<VkSemaphore>(_VKSC)
	{
	}

	~VKSemaphore();
};