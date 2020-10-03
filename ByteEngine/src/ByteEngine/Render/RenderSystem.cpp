#include "RenderSystem.h"

#include <GTSL/Window.h>


#include "MaterialSystem.h"
#include "ByteEngine/Application/Application.h"
#include "ByteEngine/Application/ThreadPool.h"
#include "ByteEngine/Debug/Assert.h"
#include "ByteEngine/Resources/PipelineCacheResourceManager.h"

#undef MemoryBarrier

class CameraSystem;
class RenderStaticMeshCollection;

void RenderSystem::InitializeRenderer(const InitializeRendererInfo& initializeRenderer)
{
	//apiAllocations.Initialize(128, GetPersistentAllocator());
	apiAllocations.reserve(16);

	rayTracingMeshes.Initialize(32, GetPersistentAllocator());
	buildAccelerationStructureInfos.Initialize(32, GetPersistentAllocator());
	buildOffsets.Initialize(32, GetPersistentAllocator());

	RenderDevice::RayTracingCapabilities rayTracingCapabilities;
	
	{		
		RenderDevice::CreateInfo createInfo;
		createInfo.ApplicationName = GTSL::StaticString<128>("Test");
		
		GTSL::Array<GAL::Queue::CreateInfo, 5> queue_create_infos(2);
		queue_create_infos[0].Capabilities = static_cast<uint8>(QueueCapabilities::GRAPHICS);
		queue_create_infos[0].QueuePriority = 1.0f;
		queue_create_infos[1].Capabilities = static_cast<uint8>(QueueCapabilities::TRANSFER);
		queue_create_infos[1].QueuePriority = 1.0f;
		createInfo.QueueCreateInfos = queue_create_infos;
		auto queues = GTSL::Array<Queue*, 5>{ &graphicsQueue, &transferQueue };
		createInfo.Queues = queues;
		createInfo.Extensions = GTSL::Array<RenderDevice::Extension, 8>{ RenderDevice::Extension::RAY_TRACING, RenderDevice::Extension::PIPELINE_CACHE_EXTERNAL_SYNC };
		createInfo.ExtensionCapabilities = GTSL::Array<void*, 8>{ &rayTracingCapabilities };
		createInfo.DebugPrintFunction = GTSL::Delegate<void(const char*, RenderDevice::MessageSeverity)>::Create<RenderSystem, &RenderSystem::printError>(this);
		createInfo.AllocationInfo.UserData = this;
		createInfo.AllocationInfo.Allocate = GTSL::Delegate<void*(void*, uint64, uint64)>::Create<RenderSystem, &RenderSystem::allocateApiMemory>(this);
		createInfo.AllocationInfo.Reallocate = GTSL::Delegate<void*(void*, void*, uint64, uint64)>::Create<RenderSystem, &RenderSystem::reallocateApiMemory>(this);
		createInfo.AllocationInfo.Deallocate = GTSL::Delegate<void(void*, void*)>::Create<RenderSystem, &RenderSystem::deallocateApiMemory>(this);
		::new(&renderDevice) RenderDevice(createInfo);

		scratchMemoryAllocator.Initialize(renderDevice, GetPersistentAllocator());
		localMemoryAllocator.Initialize(renderDevice, GetPersistentAllocator());
		
		if (createInfo.Extensions[0] == RenderDevice::Extension::RAY_TRACING)
		{
			AccelerationStructure::GeometryDescriptor descriptor;
			descriptor.Type = GeometryType::INSTANCES;
			descriptor.MaxPrimitiveCount = 1;
			descriptor.AllowTransforms = false;
			descriptor.VertexType = static_cast<ShaderDataType>(0);
			descriptor.IndexType = static_cast<IndexType>(0);
			
			AccelerationStructure::TopLevelCreateInfo accelerationStructureCreateInfo;
			accelerationStructureCreateInfo.RenderDevice = GetRenderDevice();
			accelerationStructureCreateInfo.Flags = AccelerationStructureFlags::PREFER_FAST_TRACE;
			accelerationStructureCreateInfo.GeometryDescriptors = GTSL::Range<AccelerationStructure::GeometryDescriptor*>(1, &descriptor);

			topLevelAccelerationStructure.Initialize(accelerationStructureCreateInfo);

			{
				RenderDevice::MemoryRequirements memoryRequirements;
				RenderDevice::GetAccelerationStructureMemoryRequirementsInfo accelerationStructureMemoryRequirements;
				accelerationStructureMemoryRequirements.MemoryRequirements = &memoryRequirements;
				accelerationStructureMemoryRequirements.AccelerationStructure = &topLevelAccelerationStructure;
				accelerationStructureMemoryRequirements.AccelerationStructureMemoryRequirementsType = GAL::VulkanAccelerationStructureMemoryRequirementsType::OBJECT;
				accelerationStructureMemoryRequirements.AccelerationStructureBuildType = GAL::VulkanAccelerationStructureBuildType::GPU_LOCAL;
				GetRenderDevice()->GetAccelerationStructureMemoryRequirements(accelerationStructureMemoryRequirements);

				{
					RenderAllocation allocation;

					allocation.Size = memoryRequirements.Size;
					
					AccelerationStructure::BindToMemoryInfo bindInfo;
					bindInfo.RenderDevice = GetRenderDevice();

					localMemoryAllocator.AllocateBuffer(*GetRenderDevice(), &bindInfo.Memory, &allocation, GetPersistentAllocator());

					bindInfo.Offset = allocation.Offset;

					topLevelAccelerationStructure.BindToMemory(bindInfo);

					topLevelAccelerationStructureAddress= topLevelAccelerationStructure.GetAddress(GetRenderDevice());
				}
			}

			{
				Buffer::CreateInfo buffer;
				buffer.RenderDevice = GetRenderDevice();
				buffer.Size = MAX_INSTANCES_COUNT * sizeof(AccelerationStructure::Instance);
				buffer.BufferType = BufferType::ADDRESS;
				instancesBuffer.Initialize(buffer);
				
				BufferScratchMemoryAllocationInfo allocationInfo;
				allocationInfo.Allocation = &instancesAllocation;
				allocationInfo.Buffer = instancesBuffer;
				AllocateScratchBufferMemory(allocationInfo);

				instancesBufferAddress = instancesBuffer.GetAddress(GetRenderDevice());
			}

			{				
				Buffer::CreateInfo buffer;
				buffer.RenderDevice = GetRenderDevice();
				buffer.Size = MAX_INSTANCES_COUNT * sizeof(AccelerationStructure::Instance);
				buffer.BufferType = BufferType::ADDRESS | BufferType::RAY_TRACING;
				accelerationStructureScratchBuffer.Initialize(buffer);

				BufferLocalMemoryAllocationInfo allocationInfo;
				allocationInfo.Allocation = &scratchBufferAllocation;
				allocationInfo.Buffer = accelerationStructureScratchBuffer;
				AllocateLocalBufferMemory(allocationInfo);

				scratchBufferAddress = accelerationStructureScratchBuffer.GetAddress(GetRenderDevice());
			}
		}
	}

	if (rayTracingCapabilities.CanBuildOnHost)
	{
	}
	else
	{
		buildAccelerationStructures = decltype(buildAccelerationStructures)::Create<RenderSystem, &RenderSystem::buildAccelerationStructuresOnDevice>();
	}
	
	swapchainPresentMode = PresentMode::FIFO;
	swapchainColorSpace = ColorSpace::NONLINEAR_SRGB;
	swapchainFormat = TextureFormat::BGRA_I8;


	for(uint8 i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
	{
		processedTextureCopies.EmplaceBack(0);
		processedBufferCopies.EmplaceBack(0);
	}

	{
		Semaphore::CreateInfo semaphoreCreateInfo;
		semaphoreCreateInfo.RenderDevice = GetRenderDevice();
		
		for(uint32 i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
		{
			if constexpr (_DEBUG) { semaphoreCreateInfo.Name = "Transfer semaphore"; }
			transferDoneSemaphores.EmplaceBack(semaphoreCreateInfo);
		}
	}
	
	Surface::CreateInfo surfaceCreateInfo;
	surfaceCreateInfo.RenderDevice = &renderDevice;
	if constexpr (_DEBUG) { surfaceCreateInfo.Name = "Surface"; }
	GTSL::Window::Win32NativeHandles handles;
	initializeRenderer.Window->GetNativeHandles(&handles);
	GAL::WindowsWindowData windowsWindowData;
	windowsWindowData.InstanceHandle = GetModuleHandle(NULL);
	windowsWindowData.WindowHandle = handles.HWND;
	surfaceCreateInfo.SystemData = &windowsWindowData;
	new(&surface) Surface(surfaceCreateInfo);
	
	for (uint32 i = 0; i < 2; ++i)
	{
		Semaphore::CreateInfo semaphore_create_info;
		semaphore_create_info.RenderDevice = &renderDevice;
		semaphore_create_info.Name = "ImageAvailableSemaphore";
		imageAvailableSemaphore.EmplaceBack(semaphore_create_info);
		semaphore_create_info.Name = "RenderFinishedSemaphore";
		renderFinishedSemaphore.EmplaceBack(semaphore_create_info);

		Fence::CreateInfo fence_create_info;
		fence_create_info.RenderDevice = &renderDevice;
		fence_create_info.Name = "InFlightFence";
		fence_create_info.IsSignaled = true;
		graphicsFences.EmplaceBack(fence_create_info);
		fence_create_info.Name = "TransferFence";
		fence_create_info.IsSignaled = true;
		transferFences.EmplaceBack(fence_create_info);

		{
			CommandPool::CreateInfo commandPoolCreateInfo;
			commandPoolCreateInfo.RenderDevice = &renderDevice;
			
			if constexpr (_DEBUG)
			{
				GTSL::StaticString<64> commandPoolName("Transfer command pool. Frame: "); commandPoolName += i;
				commandPoolCreateInfo.Name = commandPoolName.begin();
			}
			
			commandPoolCreateInfo.Queue = &graphicsQueue;

			graphicsCommandPools.EmplaceBack(commandPoolCreateInfo);

			CommandPool::AllocateCommandBuffersInfo allocateCommandBuffersInfo;
			allocateCommandBuffersInfo.IsPrimary = true;
			allocateCommandBuffersInfo.RenderDevice = &renderDevice;

			CommandBuffer::CreateInfo commandBufferCreateInfo;
			commandBufferCreateInfo.RenderDevice = &renderDevice;

			if constexpr (_DEBUG)
			{
				GTSL::StaticString<64> commandBufferName("Graphics command buffer. Frame: "); commandBufferName += i;
				commandBufferCreateInfo.Name = commandBufferName.begin();
			}

			GTSL::Array<CommandBuffer::CreateInfo, 5> createInfos; createInfos.EmplaceBack(commandBufferCreateInfo);
			allocateCommandBuffersInfo.CommandBufferCreateInfos = createInfos;
			graphicsCommandBuffers.Resize(graphicsCommandBuffers.GetLength() + 1);
			allocateCommandBuffersInfo.CommandBuffers = GTSL::Range<CommandBuffer*>(1, graphicsCommandBuffers.begin() + i);
			graphicsCommandPools[i].AllocateCommandBuffer(allocateCommandBuffersInfo);
		}

		{
			
			CommandPool::CreateInfo commandPoolCreateInfo;
			commandPoolCreateInfo.RenderDevice = &renderDevice;
			
			if constexpr (_DEBUG)
			{
				GTSL::StaticString<64> commandPoolName("Transfer command pool. Frame: "); commandPoolName += i;
				commandPoolCreateInfo.Name = commandPoolName.begin();
			}
			
			commandPoolCreateInfo.Queue = &transferQueue;
			transferCommandPools.EmplaceBack(commandPoolCreateInfo);

			CommandPool::AllocateCommandBuffersInfo allocate_command_buffers_info;
			allocate_command_buffers_info.RenderDevice = &renderDevice;
			allocate_command_buffers_info.IsPrimary = true;

			CommandBuffer::CreateInfo commandBufferCreateInfo;
			commandBufferCreateInfo.RenderDevice = &renderDevice;
			
			if constexpr (_DEBUG)
			{
				GTSL::StaticString<64> commandBufferName("Transfer command buffer. Frame: "); commandBufferName += i;
				commandBufferCreateInfo.Name = commandBufferName.begin();	
			}
			
			GTSL::Array<CommandBuffer::CreateInfo, 5> createInfos; createInfos.EmplaceBack(commandBufferCreateInfo);
			allocate_command_buffers_info.CommandBufferCreateInfos = createInfos;
			transferCommandBuffers.Resize(transferCommandBuffers.GetLength() + 1);
			allocate_command_buffers_info.CommandBuffers = GTSL::Range<CommandBuffer*>(1, transferCommandBuffers.begin() + i);
			transferCommandPools[i].AllocateCommandBuffer(allocate_command_buffers_info);
		}

		bufferCopyDatas.EmplaceBack(128, GetPersistentAllocator());
		textureCopyDatas.EmplaceBack(128, GetPersistentAllocator());
	}

	bool pipelineCacheAvailable;
	initializeRenderer.PipelineCacheResourceManager->DoesCacheExist(pipelineCacheAvailable);

	pipelineCaches.Initialize(BE::Application::Get()->GetNumberOfThreads(), GetPersistentAllocator());
	
	if(pipelineCacheAvailable)
	{
		uint32 cacheSize = 0;
		initializeRenderer.PipelineCacheResourceManager->GetCacheSize(cacheSize);

		GTSL::Buffer pipelineCacheBuffer;
		pipelineCacheBuffer.Allocate(cacheSize, 32, GetPersistentAllocator());

		initializeRenderer.PipelineCacheResourceManager->GetCache(pipelineCacheBuffer);
		
		PipelineCache::CreateInfo pipelineCacheCreateInfo;
		pipelineCacheCreateInfo.RenderDevice = GetRenderDevice();
		pipelineCacheCreateInfo.ExternallySync = true;
		pipelineCacheCreateInfo.Data = pipelineCacheBuffer;

		for(uint8 i = 0; i < BE::Application::Get()->GetNumberOfThreads(); ++i)
		{
			if constexpr (_DEBUG)
			{
				GTSL::StaticString<32> name("Pipeline cache. Thread: "); name += i;
				pipelineCacheCreateInfo.Name = name.begin();
			}
			
			pipelineCaches.EmplaceBack(pipelineCacheCreateInfo);
		}
		
		pipelineCacheBuffer.Free(32, GetPersistentAllocator());
	}
	else
	{
		PipelineCache::CreateInfo pipelineCacheCreateInfo;
		pipelineCacheCreateInfo.RenderDevice = GetRenderDevice();
		pipelineCacheCreateInfo.ExternallySync = false;
		
		for (uint8 i = 0; i < BE::Application::Get()->GetNumberOfThreads(); ++i)
		{
			if constexpr (_DEBUG)
			{
				GTSL::StaticString<32> name("Pipeline cache. Thread: "); name += i;
				pipelineCacheCreateInfo.Name = name.begin();
			}
			
			pipelineCaches.EmplaceBack(pipelineCacheCreateInfo);
		}
	}
	
	BE_LOG_MESSAGE("Initialized successfully");
}

const PipelineCache* RenderSystem::GetPipelineCache() const { return &pipelineCaches[GTSL::Thread::ThisTreadID()]; }

ComponentReference RenderSystem::CreateRayTracedMesh(const CreateRayTracingMeshInfo& info)
{
	const auto component = rayTracingMeshes.GetFirstFreeIndex().Get();

	RayTracingMesh rayTracingMesh;
	rayTracingMesh.Buffer = info.Buffer;
	rayTracingMesh.IndexType = info.IndexType;
	rayTracingMesh.IndicesCount = info.IndexCount;
	rayTracingMesh.IndicesOffset = info.IndicesOffset;
	
	GAL::BuildOffset offset;
	offset.FirstVertex = 0;
	offset.PrimitiveCount = rayTracingMesh.IndicesCount / 3;
	offset.PrimitiveOffset = 0;
	offset.TransformOffset = 0;
	buildOffsets.EmplaceBack(offset);

	AccelerationStructure::GeometryDescriptor geometryDescriptor;
	geometryDescriptor.Type = GeometryType::TRIANGLES;
	geometryDescriptor.IndexType = rayTracingMesh.IndexType;
	geometryDescriptor.VertexType = ShaderDataType::FLOAT3;
	geometryDescriptor.MaxVertexCount = info.Vertices;
	geometryDescriptor.MaxPrimitiveCount = info.IndexCount / 3;
	geometryDescriptor.AllowTransforms = false;
	
	AccelerationStructure::BottomLevelCreateInfo accelerationStructureCreateInfo;
	accelerationStructureCreateInfo.RenderDevice = GetRenderDevice();
	accelerationStructureCreateInfo.Flags = AccelerationStructureFlags::PREFER_FAST_TRACE;
	accelerationStructureCreateInfo.GeometryDescriptors = GTSL::Range<AccelerationStructure::GeometryDescriptor*>(1, &geometryDescriptor);
	accelerationStructureCreateInfo.DeviceAddress = 0;
	accelerationStructureCreateInfo.CompactedSize = 0;

	rayTracingMesh.AccelerationStructure.Initialize(accelerationStructureCreateInfo);

	RenderDevice::MemoryRequirements memoryRequirements;
	RenderDevice::GetAccelerationStructureMemoryRequirementsInfo accelerationStructureMemoryRequirements;
	accelerationStructureMemoryRequirements.MemoryRequirements = &memoryRequirements;
	accelerationStructureMemoryRequirements.AccelerationStructure = &rayTracingMesh.AccelerationStructure;
	accelerationStructureMemoryRequirements.AccelerationStructureMemoryRequirementsType = GAL::VulkanAccelerationStructureMemoryRequirementsType::OBJECT;
	accelerationStructureMemoryRequirements.AccelerationStructureBuildType = GAL::VulkanAccelerationStructureBuildType::GPU_LOCAL;
	GetRenderDevice()->GetAccelerationStructureMemoryRequirements(accelerationStructureMemoryRequirements);

	{
		RenderAllocation allocation;
		
		AccelerationStructure::BindToMemoryInfo bindInfo;
		bindInfo.RenderDevice = GetRenderDevice();
		
		localMemoryAllocator.AllocateBuffer(*GetRenderDevice(), &bindInfo.Memory, &allocation, GetPersistentAllocator());

		bindInfo.Offset = allocation.Offset;
		
		rayTracingMesh.AccelerationStructure.BindToMemory(bindInfo);

		rayTracingMesh.Address = rayTracingMesh.AccelerationStructure.GetAddress(GetRenderDevice());
	}

	{
		AccelerationStructure::GeometryTriangleData triangleData;
		triangleData.VertexType = ShaderDataType::FLOAT3;
		triangleData.VertexStride = sizeof(GTSL::Vector3);
		triangleData.VertexBufferAddress = info.Buffer.GetAddress(GetRenderDevice());
		triangleData.IndexType = rayTracingMesh.IndexType;
		triangleData.IndexBufferAddress = triangleData.VertexBufferAddress + info.IndicesOffset;

		triangleDatas.emplace_back(triangleData);
	}

	rayTracingMeshes.Emplace(rayTracingMesh);


	{
		auto& instance = *(static_cast<AccelerationStructure::Instance*>(instancesAllocation.Data) + instanceCount);
		
		instance.Flags = GeometryInstanceFlags::DISABLE_CULLING | GeometryInstanceFlags::OPAQUE;
		instance.AccelerationStructureReference = reinterpret_cast<uint64>(rayTracingMesh.AccelerationStructure.GetVkAccelerationStructure());
		instance.Mask = 0xFF;
		instance.InstanceCustomIndex = 0;
		instance.InstanceShaderBindingTableRecordOffset = 0;
		instance.Transform = *info.Matrix;

		BE_ASSERT(instanceCount < MAX_INSTANCES_COUNT);
		
		++instanceCount;
	}
	
	return component;
}

void RenderSystem::OnResize(const GTSL::Extent2D extent)
{
	graphicsQueue.Wait(GetRenderDevice());

	BE_ASSERT(surface.IsSupported(&renderDevice) != false, "Surface is not supported!");

	GTSL::Array<PresentMode, 4> present_modes{ swapchainPresentMode };
	auto res = surface.GetSupportedPresentMode(&renderDevice, present_modes);
	if (res != 0xFFFFFFFF) { swapchainPresentMode = present_modes[res]; }

	GTSL::Array<GTSL::Pair<ColorSpace, TextureFormat>, 8> surface_formats{ { swapchainColorSpace, swapchainFormat } };
	res = surface.GetSupportedRenderContextFormat(&renderDevice, surface_formats);
	if (res != 0xFFFFFFFF) { swapchainColorSpace = surface_formats[res].First; swapchainFormat = surface_formats[res].Second; }
	
	RenderContext::RecreateInfo recreate;
	recreate.RenderDevice = GetRenderDevice();
	if constexpr (_DEBUG)
	{
		GTSL::StaticString<64> name("Swapchain");
		recreate.Name = name.begin();
	}
	recreate.SurfaceArea = extent;
	recreate.ColorSpace = swapchainColorSpace;
	recreate.DesiredFramesInFlight = 2;
	recreate.Format = swapchainFormat;
	recreate.PresentMode = swapchainPresentMode;
	recreate.Surface = &surface;
	recreate.TextureUses = TextureUses::COLOR_ATTACHMENT | TextureUses::TRANSFER_DESTINATION;
	recreate.Queue = &graphicsQueue;
	renderContext.Recreate(recreate);
	
	for (auto& e : swapchainTextureViews) { e.Destroy(&renderDevice); }

	RenderContext::GetTexturesInfo getTexturesInfo;
	getTexturesInfo.RenderDevice = GetRenderDevice();
	swapchainTextures = renderContext.GetTextures(getTexturesInfo);
	
	RenderContext::GetTextureViewsInfo getTextureViewsInfo;
	getTextureViewsInfo.RenderDevice = &renderDevice;
	GTSL::Array<TextureView::CreateInfo, MAX_CONCURRENT_FRAMES> textureViewCreateInfos(MAX_CONCURRENT_FRAMES);
	{
		for (uint8 i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
		{
			textureViewCreateInfos[i].RenderDevice = GetRenderDevice();
			if constexpr (_DEBUG)
			{
				GTSL::StaticString<64> name("Swapchain texture view. Frame: "); name += static_cast<uint16>(i); //cast to not consider it a char
				textureViewCreateInfos[i].Name = name.begin();
			}
			textureViewCreateInfos[i].Format = swapchainFormat;
		}
	}
	getTextureViewsInfo.TextureViewCreateInfos = textureViewCreateInfos;

	{
		swapchainTextureViews = renderContext.GetTextureViews(getTextureViewsInfo);
	}

	renderArea = extent;
	
	BE_LOG_MESSAGE("Resized window")
}

void RenderSystem::Initialize(const InitializeInfo& initializeInfo)
{
	{
		const GTSL::Array<TaskDependency, 8> actsOn{ { "RenderSystem", AccessType::READ_WRITE } };
		initializeInfo.GameInstance->AddTask("frameStart", GTSL::Delegate<void(TaskInfo)>::Create<RenderSystem, &RenderSystem::frameStart>(this), actsOn, "FrameStart", "RenderStart");

		initializeInfo.GameInstance->AddTask("executeTransfers", GTSL::Delegate<void(TaskInfo)>::Create<RenderSystem, &RenderSystem::executeTransfers>(this), actsOn, "GameplayEnd", "RenderStart");
	}

	{
		const GTSL::Array<TaskDependency, 8> actsOn{ { "RenderSystem", AccessType::READ_WRITE } };
		initializeInfo.GameInstance->AddTask("renderStart", GTSL::Delegate<void(TaskInfo)>::Create<RenderSystem, &RenderSystem::renderStart>(this), actsOn, "RenderStart", "RenderStartSetup");
		initializeInfo.GameInstance->AddTask("renderSetup", GTSL::Delegate<void(TaskInfo)>::Create<RenderSystem, &RenderSystem::renderBegin>(this), actsOn, "RenderEndSetup", "RenderDo");
	}

	{
		const GTSL::Array<TaskDependency, 8> actsOn{ { "RenderSystem", AccessType::READ_WRITE } };
		initializeInfo.GameInstance->AddTask("renderFinished", GTSL::Delegate<void(TaskInfo)>::Create<RenderSystem, &RenderSystem::renderFinish>(this), actsOn, "RenderFinished", "RenderEnd");
	}
}

void RenderSystem::Shutdown(const ShutdownInfo& shutdownInfo)
{
	Wait();
	
	for (uint32 i = 0; i < swapchainTextures.GetLength(); ++i)
	{
		CommandPool::FreeCommandBuffersInfo free_command_buffers_info;
		free_command_buffers_info.RenderDevice = &renderDevice;

		free_command_buffers_info.CommandBuffers = GTSL::Range<CommandBuffer*>(1, &graphicsCommandBuffers[i]);
		graphicsCommandPools[i].FreeCommandBuffers(free_command_buffers_info);

		free_command_buffers_info.CommandBuffers = GTSL::Range<CommandBuffer*>(1, &transferCommandBuffers[i]);
		transferCommandPools[i].FreeCommandBuffers(free_command_buffers_info);

		graphicsCommandPools[i].Destroy(&renderDevice);
		transferCommandPools[i].Destroy(&renderDevice);
	}
	
	renderContext.Destroy(&renderDevice);
	surface.Destroy(&renderDevice);
	
	//DeallocateLocalTextureMemory();
	
	for(auto& e : imageAvailableSemaphore) { e.Destroy(&renderDevice); }
	for(auto& e : renderFinishedSemaphore) { e.Destroy(&renderDevice); }
	for(auto& e : graphicsFences) { e.Destroy(&renderDevice); }
	for(auto& e : transferFences) { e.Destroy(&renderDevice); }

	for (auto& e : swapchainTextureViews) { e.Destroy(&renderDevice); }

	scratchMemoryAllocator.Free(renderDevice, GetPersistentAllocator());
	localMemoryAllocator.Free(renderDevice, GetPersistentAllocator());

	{
		uint32 cacheSize = 0;

		PipelineCache::CreateFromMultipleInfo createPipelineCacheInfo;
		createPipelineCacheInfo.RenderDevice = GetRenderDevice();
		createPipelineCacheInfo.Caches = pipelineCaches;
		const PipelineCache pipelineCache(createPipelineCacheInfo);
		pipelineCache.GetCacheSize(GetRenderDevice(), cacheSize);

		if (cacheSize)
		{
			auto* pipelineCacheResourceManager = BE::Application::Get()->GetResourceManager<PipelineCacheResourceManager>("PipelineCacheResourceManager");
			GTSL::Buffer pipelineCacheBuffer;
			pipelineCacheBuffer.Allocate(cacheSize, 32, GetPersistentAllocator());
			pipelineCache.GetCache(&renderDevice, cacheSize, pipelineCacheBuffer);
			pipelineCacheResourceManager->WriteCache(pipelineCacheBuffer);
			pipelineCacheBuffer.Free(32, GetPersistentAllocator());
		}
	}
}

void RenderSystem::Wait()
{
	graphicsQueue.Wait(GetRenderDevice());
	transferQueue.Wait(GetRenderDevice());
}

void RenderSystem::renderStart(TaskInfo taskInfo)
{
	//Fence::WaitForFencesInfo waitForFencesInfo;
	//waitForFencesInfo.RenderDevice = &renderDevice;
	//waitForFencesInfo.Timeout = ~0ULL;
	//waitForFencesInfo.WaitForAll = true;
	//waitForFencesInfo.Fences = GTSL::Range<const Fence*>(1, &graphicsFences[currentFrameIndex]);
	//Fence::WaitForFences(waitForFencesInfo);

	graphicsFences[currentFrameIndex].Wait(GetRenderDevice());

	//Fence::ResetFencesInfo resetFencesInfo;
	//resetFencesInfo.RenderDevice = &renderDevice;
	//resetFencesInfo.Fences = GTSL::Range<const Fence*>(1, &graphicsFences[currentFrameIndex]);
	//Fence::ResetFences(resetFencesInfo);
	
	graphicsFences[currentFrameIndex].Reset(GetRenderDevice());
	
	graphicsCommandPools[currentFrameIndex].ResetPool(&renderDevice);
}

void RenderSystem::buildAccelerationStructuresOnDevice()
{
	auto& commandBuffer = transferCommandBuffers[GetCurrentFrame()];
	
	GTSL::Array<GAL::BuildOffset*, 10> bO; bO.EmplaceBack(buildOffsets.GetData());

	GAL::BuildAccelerationStructuresInfo build;
	build.RenderDevice = GetRenderDevice();
	build.BuildAccelerationStructureInfos = buildAccelerationStructureInfos;
	build.BuildOffsets = bO.begin();

	commandBuffer.BuildAccelerationStructure(build);

	CommandBuffer::AddPipelineBarrierInfo addPipelineBarrierInfo;
	addPipelineBarrierInfo.InitialStage = PipelineStage::ACCELERATION_STRUCTURE_BUILD;
	addPipelineBarrierInfo.FinalStage = PipelineStage::ACCELERATION_STRUCTURE_BUILD;

	GTSL::Array<CommandBuffer::MemoryBarrier, 1> memoryBarriers(1);
	memoryBarriers[0].SourceAccessFlags = AccessFlags::ACCELERATION_STRUCTURE_WRITE;
	memoryBarriers[0].DestinationAccessFlags = AccessFlags::ACCELERATION_STRUCTURE_READ;

	addPipelineBarrierInfo.MemoryBarriers = memoryBarriers;
	commandBuffer.AddPipelineBarrier(addPipelineBarrierInfo);
}

void RenderSystem::renderBegin(TaskInfo taskInfo)
{	
	auto& commandBuffer = graphicsCommandBuffers[currentFrameIndex];
	
	commandBuffer.BeginRecording({});
}

void RenderSystem::renderFinish(TaskInfo taskInfo)
{
	auto& commandBuffer = graphicsCommandBuffers[currentFrameIndex];

	commandBuffer.EndRecording({});
	
	RenderContext::AcquireNextImageInfo acquireNextImageInfo;
	acquireNextImageInfo.RenderDevice = &renderDevice;
	acquireNextImageInfo.SignalSemaphore = &imageAvailableSemaphore[currentFrameIndex];
	auto imageIndex = renderContext.AcquireNextImage(acquireNextImageInfo);

	//BE_ASSERT(imageIndex == currentFrameIndex, "Data mismatch");
	
	Queue::SubmitInfo submitInfo;
	submitInfo.RenderDevice = &renderDevice;
	submitInfo.Fence = &graphicsFences[currentFrameIndex];
	submitInfo.WaitSemaphores = GTSL::Array<Semaphore, 2>{ imageAvailableSemaphore[currentFrameIndex], transferDoneSemaphores[GetCurrentFrame()] };
	submitInfo.SignalSemaphores = GTSL::Range<const Semaphore*>(1, &renderFinishedSemaphore[currentFrameIndex]);
	submitInfo.CommandBuffers = GTSL::Range<const CommandBuffer*>(1, &commandBuffer);
	GTSL::Array<uint32, 8> wps{ PipelineStage::COLOR_ATTACHMENT_OUTPUT, PipelineStage::TRANSFER };
	submitInfo.WaitPipelineStages = wps;
	graphicsQueue.Submit(submitInfo);

	RenderContext::PresentInfo presentInfo;
	presentInfo.RenderDevice = &renderDevice;
	presentInfo.Queue = &graphicsQueue;
	presentInfo.WaitSemaphores = GTSL::Range<const Semaphore*>(1, &renderFinishedSemaphore[currentFrameIndex]);
	presentInfo.ImageIndex = imageIndex;
	renderContext.Present(presentInfo);

	currentFrameIndex = (currentFrameIndex + 1) % swapchainTextureViews.GetLength();
}

void RenderSystem::frameStart(TaskInfo taskInfo)
{
	Fence::WaitForFencesInfo wait_for_fences_info;
	wait_for_fences_info.RenderDevice = &renderDevice;
	wait_for_fences_info.Timeout = ~0ULL;
	wait_for_fences_info.WaitForAll = true;
	wait_for_fences_info.Fences = GTSL::Range<const Fence*>(1, &transferFences[currentFrameIndex]);
	Fence::WaitForFences(wait_for_fences_info);

	auto& bufferCopyData = bufferCopyDatas[GetCurrentFrame()];
	auto& textureCopyData = textureCopyDatas[GetCurrentFrame()];
	
	//if(transferFences[currentFrameIndex].GetStatus(&renderDevice))
	{
		for(uint32 i = 0; i < processedBufferCopies[GetCurrentFrame()]; ++i)
		{
			bufferCopyData[i].SourceBuffer.Destroy(&renderDevice);
			DeallocateScratchBufferMemory(bufferCopyData[i].Allocation);
		}

		for(uint32 i = 0; i < processedTextureCopies[GetCurrentFrame()]; ++i)
		{
			textureCopyData[i].SourceBuffer.Destroy(&renderDevice);
			DeallocateScratchBufferMemory(textureCopyData[i].Allocation);
		}
		
		bufferCopyData.Pop(0, processedBufferCopies[GetCurrentFrame()]);
		textureCopyData.Pop(0, processedTextureCopies[GetCurrentFrame()]);
		buildAccelerationStructureInfos.ResizeDown(0);
		buildOffsets.ResizeDown(0);
		geometries.clear();
		triangleDatas.clear();

		Fence::ResetFencesInfo reset_fences_info;
		reset_fences_info.RenderDevice = &renderDevice;
		reset_fences_info.Fences = GTSL::Range<const Fence*>(1, &transferFences[currentFrameIndex]);
		Fence::ResetFences(reset_fences_info);
	}
	
	transferCommandPools[currentFrameIndex].ResetPool(&renderDevice); //should only be done if frame is finished transferring but must also implement check in execute transfers
	//or begin command buffer complains
}

void RenderSystem::executeTransfers(TaskInfo taskInfo)
{
	auto& commandBuffer = transferCommandBuffers[GetCurrentFrame()];
	
	CommandBuffer::BeginRecordingInfo beginRecordingInfo;
	beginRecordingInfo.RenderDevice = &renderDevice;
	commandBuffer.BeginRecording(beginRecordingInfo);

	{
		auto& bufferCopyData = bufferCopyDatas[GetCurrentFrame()];
		
		for (auto& e : bufferCopyData)
		{
			CommandBuffer::CopyBuffersInfo copy_buffers_info;
			copy_buffers_info.RenderDevice = &renderDevice;
			copy_buffers_info.Destination = &e.DestinationBuffer;
			copy_buffers_info.DestinationOffset = e.DestinationOffset;
			copy_buffers_info.Source = &e.SourceBuffer;
			copy_buffers_info.SourceOffset = e.SourceOffset;
			copy_buffers_info.Size = e.Size;
			commandBuffer.CopyBuffers(copy_buffers_info);
		}

		processedBufferCopies[GetCurrentFrame()] = bufferCopyData.GetLength();
	}
	
	{
		auto& textureCopyData = textureCopyDatas[GetCurrentFrame()];
		
		GTSL::Vector<CommandBuffer::TextureBarrier, BE::TransientAllocatorReference> sourceTextureBarriers(textureCopyData.GetLength(), textureCopyData.GetLength(), GetTransientAllocator());
		GTSL::Vector<CommandBuffer::TextureBarrier, BE::TransientAllocatorReference> destinationTextureBarriers(textureCopyData.GetLength(), textureCopyData.GetLength(), GetTransientAllocator());

		for (uint32 i = 0; i < textureCopyData.GetLength(); ++i)
		{
			sourceTextureBarriers[i].Texture = textureCopyData[i].DestinationTexture;
			sourceTextureBarriers[i].SourceAccessFlags = 0;
			sourceTextureBarriers[i].DestinationAccessFlags = AccessFlags::TRANSFER_WRITE;
			sourceTextureBarriers[i].CurrentLayout = TextureLayout::UNDEFINED;
			sourceTextureBarriers[i].TargetLayout = TextureLayout::TRANSFER_DST;

			destinationTextureBarriers[i].Texture = textureCopyData[i].DestinationTexture;
			destinationTextureBarriers[i].SourceAccessFlags = AccessFlags::TRANSFER_WRITE;
			destinationTextureBarriers[i].DestinationAccessFlags = AccessFlags::SHADER_READ;
			destinationTextureBarriers[i].CurrentLayout = TextureLayout::TRANSFER_DST;
			destinationTextureBarriers[i].TargetLayout = TextureLayout::SHADER_READ_ONLY;
		}


		CommandBuffer::AddPipelineBarrierInfo pipelineBarrierInfo;
		pipelineBarrierInfo.RenderDevice = GetRenderDevice();
		pipelineBarrierInfo.TextureBarriers = sourceTextureBarriers;
		pipelineBarrierInfo.InitialStage = PipelineStage::TOP_OF_PIPE;
		pipelineBarrierInfo.FinalStage = PipelineStage::TRANSFER;
		commandBuffer.AddPipelineBarrier(pipelineBarrierInfo);

		for (uint32 i = 0; i < textureCopyData.GetLength(); ++i)
		{
			CommandBuffer::CopyBufferToTextureInfo copyBufferToImageInfo;
			copyBufferToImageInfo.RenderDevice = GetRenderDevice();
			copyBufferToImageInfo.DestinationTexture = &textureCopyData[i].DestinationTexture;
			copyBufferToImageInfo.Offset = { 0, 0, 0 };
			copyBufferToImageInfo.Extent = textureCopyData[i].Extent;
			copyBufferToImageInfo.SourceBuffer = &textureCopyData[i].SourceBuffer;
			copyBufferToImageInfo.TextureLayout = textureCopyData[i].Layout;
			commandBuffer.CopyBufferToTexture(copyBufferToImageInfo);
		}
			
		pipelineBarrierInfo.TextureBarriers = destinationTextureBarriers;
		pipelineBarrierInfo.InitialStage = PipelineStage::TRANSFER;
		pipelineBarrierInfo.FinalStage = PipelineStage::ALL_COMMANDS;
		commandBuffer.AddPipelineBarrier(pipelineBarrierInfo);

		processedTextureCopies[GetCurrentFrame()] = textureCopyData.GetLength();
	}

	buildAccelerationStructures(this);
	
	CommandBuffer::EndRecordingInfo endRecordingInfo;
	endRecordingInfo.RenderDevice = &renderDevice;
	commandBuffer.EndRecording(endRecordingInfo);
	
	//if (bufferCopyDatas[currentFrameIndex].GetLength() || textureCopyDatas[GetCurrentFrame()].GetLength())
	//{
		Queue::SubmitInfo submit_info;
		submit_info.RenderDevice = &renderDevice;
		submit_info.Fence = &transferFences[currentFrameIndex];
		submit_info.CommandBuffers = GTSL::Range<const CommandBuffer*>(1, &commandBuffer);
		submit_info.WaitPipelineStages = GTSL::Array<uint32, 2>{ PipelineStage::TRANSFER };
		submit_info.SignalSemaphores = GTSL::Array<Semaphore, 1>{ transferDoneSemaphores[GetCurrentFrame()] };
		transferQueue.Submit(submit_info);
	//}
}

void RenderSystem::printError(const char* message, const RenderDevice::MessageSeverity messageSeverity) const
{
	switch (messageSeverity)
	{
	case RenderDevice::MessageSeverity::MESSAGE: BE_LOG_MESSAGE(message) break;
	case RenderDevice::MessageSeverity::WARNING: BE_LOG_WARNING(message) break;
	case RenderDevice::MessageSeverity::ERROR:   BE_LOG_ERROR(message) break;
	default: break;
	}
}

void* RenderSystem::allocateApiMemory(void* data, const uint64 size, const uint64 alignment)
{
	void* allocation; uint64 allocated_size;
	GetPersistentAllocator().Allocate(size, alignment, &allocation, &allocated_size);
	//apiAllocations.Emplace(reinterpret_cast<uint64>(allocation), size, alignment);
	{
		GTSL::Lock lock(allocationsMutex);

		BE_ASSERT(!apiAllocations.contains(reinterpret_cast<uint64>(allocation)), "")
		apiAllocations.emplace(reinterpret_cast<uint64>(allocation), GTSL::Pair<uint64, uint64>(size, alignment));
	}
	return allocation;
}

void* RenderSystem::reallocateApiMemory(void* data, void* oldAllocation, uint64 size, uint64 alignment)
{
	void* allocation; uint64 allocated_size;

	GTSL::Pair<uint64, uint64> old_alloc;
	
	{
		GTSL::Lock lock(allocationsMutex);
		//const auto old_alloc = apiAllocations.At(reinterpret_cast<uint64>(oldAllocation));
		old_alloc = apiAllocations.at(reinterpret_cast<uint64>(oldAllocation));
	}
	
	GetPersistentAllocator().Allocate(size, old_alloc.Second, &allocation, &allocated_size);
	//apiAllocations.Emplace(reinterpret_cast<uint64>(allocation), size, alignment);
	apiAllocations.emplace(reinterpret_cast<uint64>(allocation), GTSL::Pair<uint64, uint64>(size, alignment));
	
	GTSL::MemCopy(old_alloc.First, oldAllocation, allocation);
	
	GetPersistentAllocator().Deallocate(old_alloc.First, old_alloc.Second, oldAllocation);
	//apiAllocations.Remove(reinterpret_cast<uint64>(oldAllocation));
	{
		GTSL::Lock lock(allocationsMutex);
		apiAllocations.erase(reinterpret_cast<uint64>(oldAllocation));
	}
	
	return allocation;
}

void RenderSystem::deallocateApiMemory(void* data, void* allocation)
{
	GTSL::Pair<uint64, uint64> old_alloc;
	
	{
		GTSL::Lock lock(allocationsMutex);
		old_alloc = apiAllocations.at(reinterpret_cast<uint64>(allocation));
		//const auto old_alloc = apiAllocations.At(reinterpret_cast<uint64>(allocation));
	}
	
	GetPersistentAllocator().Deallocate(old_alloc.First, old_alloc.Second, allocation);
	
	{
		GTSL::Lock lock(allocationsMutex);
		apiAllocations.erase(reinterpret_cast<uint64>(allocation));
		//apiAllocations.Remove(reinterpret_cast<uint64>(allocation));
	}
}
