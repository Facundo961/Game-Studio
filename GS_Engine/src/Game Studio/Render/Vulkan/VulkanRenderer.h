#pragma once

#include "Core.h"

#include "../Renderer.h"

#include "VulkanBase.h"

#include "FVector.hpp"

#include "VulkanFramebuffer.h"

MAKE_VK_HANDLE(VkInstance)

GS_CLASS Vulkan_Instance
{
	VkInstance Instance = nullptr;
public:
	Vulkan_Instance(const char* _AppName);
	~Vulkan_Instance();

	INLINE VkInstance GetVkInstance() const { return Instance; }
};

MAKE_VK_HANDLE(VkQueue)
MAKE_VK_HANDLE(VkPhysicalDevice)
struct VkDeviceQueueCreateInfo;
struct QueueInfo;
enum VkPhysicalDeviceType;
struct VkPhysicalDeviceMemoryProperties;

GS_CLASS Vulkan_Device
{
	VkDevice Device = nullptr;
	VkQueue GraphicsQueue;
	uint32 GraphicsQueueIndex;
	VkQueue ComputeQueue;
	VkQueue TransferQueue;
	VkPhysicalDevice PhysicalDevice = nullptr;

	VkPhysicalDeviceMemoryProperties MemoryProperties = {};

	static void CreateQueueInfo(QueueInfo& _DQCI, VkPhysicalDevice _PD);
	static void CreatePhysicalDevice(VkPhysicalDevice _PD, VkInstance _Instance);
	static uint8 GetDeviceTypeScore(VkPhysicalDeviceType _Type);
public:
	Vulkan_Device(VkInstance _Instance);
	~Vulkan_Device();

	uint32 FindMemoryType(uint32 _TypeFilter, uint32 _Properties) const;
	INLINE VkDevice GetVkDevice() const { return Device; }
	INLINE VkPhysicalDevice GetVkPhysicalDevice() const { return PhysicalDevice; }
	INLINE VkQueue GetGraphicsQueue() const { return GraphicsQueue; }
	INLINE uint32 GetGraphicsQueueIndex() const { return GraphicsQueueIndex; }
	INLINE VkQueue GetComputeQueue() const { return ComputeQueue; }
	INLINE VkQueue GetTransferQueue() const { return TransferQueue; }
};

GS_CLASS VulkanRenderer final : public Renderer
{
	Vulkan_Instance Instance;
	Vulkan_Device Device;

	FVector<VulkanFramebuffer> Framebuffers;
public:
	VulkanRenderer();
	~VulkanRenderer();

	Shader* CreateShader(const ShaderCreateInfo& _SI) final override;
	Buffer* CreateBuffer(const BufferCreateInfo& _BCI) final override;
	GraphicsPipeline* CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& _GPCI) final override;
	RenderPass* CreateRenderPass(const RenderPassCreateInfo& _RPCI) final override;
	ComputePipeline* CreateComputePipeline(const ComputePipelineCreateInfo& _CPCI) final override;
	uint8 CreateFramebuffer(const FramebufferCreateInfo& _FCI) final override;
	void DestroyFramebuffer(uint8 _Handle) final override;

	INLINE const Vulkan_Device& GetVulkanDevice() const { return Device; }
	INLINE VkDevice GetVkDevice() const { return Device.GetVkDevice(); }
};

#define VKRAPI SCAST(VulkanRenderer*, Renderer::GetRenderer())