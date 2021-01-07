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
	geometries.Initialize(32, GetPersistentAllocator());
	sharedMeshes.Initialize(32, GetPersistentAllocator());
	gpuMeshes.Initialize(32, GetPersistentAllocator());
	
	RenderDevice::RayTracingCapabilities rayTracingCapabilities;

	bool rayTracing = BE::Application::Get()->GetOption("rayTracing");
	
	{		
		RenderDevice::CreateInfo createInfo;
		createInfo.ApplicationName = GTSL::StaticString<128>("Test");
		createInfo.ApplicationVersion[0] = 0;
		createInfo.ApplicationVersion[1] = 0;
		createInfo.ApplicationVersion[2] = 0;

		createInfo.Debug = BE::Application::Get()->GetOption("debug");
		
		GTSL::Array<GAL::Queue::CreateInfo, 5> queue_create_infos(2);
		queue_create_infos[0].Capabilities = QueueCapabilities::GRAPHICS;
		queue_create_infos[0].QueuePriority = 1.0f;
		queue_create_infos[1].Capabilities = QueueCapabilities::TRANSFER;
		queue_create_infos[1].QueuePriority = 1.0f;
		createInfo.QueueCreateInfos = queue_create_infos;
		auto queues = GTSL::Array<Queue*, 5>{ &graphicsQueue, &transferQueue };
		createInfo.Queues = queues;

		GTSL::Array<GTSL::Pair<RenderDevice::Extension, void*>, 8> extensions{ { RenderDevice::Extension::PIPELINE_CACHE_EXTERNAL_SYNC, nullptr } };
		if (rayTracing) { extensions.EmplaceBack(RenderDevice::Extension::RAY_TRACING, &rayTracingCapabilities); }
		
		createInfo.Extensions = extensions;
		createInfo.DebugPrintFunction = GTSL::Delegate<void(const char*, RenderDevice::MessageSeverity)>::Create<RenderSystem, &RenderSystem::printError>(this);
		createInfo.AllocationInfo.UserData = this;
		createInfo.AllocationInfo.Allocate = GTSL::Delegate<void*(void*, uint64, uint64)>::Create<RenderSystem, &RenderSystem::allocateApiMemory>(this);
		createInfo.AllocationInfo.Reallocate = GTSL::Delegate<void*(void*, void*, uint64, uint64)>::Create<RenderSystem, &RenderSystem::reallocateApiMemory>(this);
		createInfo.AllocationInfo.Deallocate = GTSL::Delegate<void(void*, void*)>::Create<RenderSystem, &RenderSystem::deallocateApiMemory>(this);
		::new(&renderDevice) RenderDevice(createInfo);

		scratchMemoryAllocator.Initialize(renderDevice, GetPersistentAllocator());
		localMemoryAllocator.Initialize(renderDevice, GetPersistentAllocator());
		
		if (rayTracing)
		{
			buildDatas.Initialize(16, GetPersistentAllocator());

			AccelerationStructure::Geometry geometry;
			geometry.PrimitiveCount = MAX_INSTANCES_COUNT;
			geometry.Flags = 0; geometry.PrimitiveOffset = 0;
			geometry.SetGeometryInstances(AccelerationStructure::GeometryInstances{ 0 });
			
			AccelerationStructure::CreateInfo accelerationStructureCreateInfo;
			accelerationStructureCreateInfo.RenderDevice = GetRenderDevice();
			accelerationStructureCreateInfo.Geometries = GTSL::Range<const AccelerationStructure::Geometry*>(1, &geometry);

			AllocateAccelerationStructureMemory(&topLevelAccelerationStructure, &topLevelAccelerationStructureBuffer, 
				GTSL::Range<const AccelerationStructure::Geometry*>(1, &geometry), &accelerationStructureCreateInfo, &topLevelAccelerationStructureAllocation,
				BuildType::GPU_LOCAL);

			{
				topLevelAccelerationStructureAddress = topLevelAccelerationStructure.GetAddress(GetRenderDevice());
			}

			{
				Buffer::CreateInfo buffer;
				buffer.RenderDevice = GetRenderDevice();
				buffer.Size = MAX_INSTANCES_COUNT * sizeof(AccelerationStructure::Instance);
				buffer.BufferType = BufferType::ADDRESS;
				
				BufferScratchMemoryAllocationInfo allocationInfo;
				allocationInfo.Allocation = &instancesAllocation;
				allocationInfo.Buffer = &instancesBuffer;
				allocationInfo.CreateInfo = &buffer;
				AllocateScratchBufferMemory(allocationInfo);

				instancesBufferAddress = instancesBuffer.GetAddress(GetRenderDevice());
			}

			{				
				Buffer::CreateInfo buffer;
				buffer.RenderDevice = GetRenderDevice();
				buffer.Size = GTSL::Byte(GTSL::MegaByte(1));
				buffer.BufferType = BufferType::ADDRESS | BufferType::STORAGE;

				BufferLocalMemoryAllocationInfo allocationInfo;
				allocationInfo.Allocation = &scratchBufferAllocation;
				allocationInfo.CreateInfo = &buffer;
				allocationInfo.Buffer = &accelerationStructureScratchBuffer;
				AllocateLocalBufferMemory(allocationInfo);

				scratchBufferAddress = accelerationStructureScratchBuffer.GetAddress(GetRenderDevice());
			}

			shaderGroupAlignment = rayTracingCapabilities.ShaderGroupAlignment;
			shaderGroupHandleSize = rayTracingCapabilities.ShaderGroupHandleSize;
			scratchBufferOffsetAlignment = rayTracingCapabilities.ScratchBuildOffsetAlignment;
		}
		else
		{
			rayTracingCapabilities.CanBuildOnHost = false;
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
			if constexpr (_DEBUG) { GTSL::StaticString<32> name("Transfer semaphore. Frame: "); name += i;  semaphoreCreateInfo.Name = name; }
			transferDoneSemaphores.EmplaceBack(semaphoreCreateInfo);
		}
	}
	
	Surface::CreateInfo surfaceCreateInfo;
	surfaceCreateInfo.RenderDevice = &renderDevice;
	if constexpr (_DEBUG) { surfaceCreateInfo.Name = GTSL::StaticString<32>("Surface"); }
	GTSL::Window::Win32NativeHandles handles;
	initializeRenderer.Window->GetNativeHandles(&handles);
	GAL::WindowsWindowData windowsWindowData;
	windowsWindowData.InstanceHandle = GetModuleHandle(nullptr);
	windowsWindowData.WindowHandle = handles.HWND;
	surfaceCreateInfo.SystemData = &windowsWindowData;
	new(&surface) Surface(surfaceCreateInfo);
	
	for (uint32 i = 0; i < 2; ++i)
	{
		Semaphore::CreateInfo semaphoreCreateInfo;
		semaphoreCreateInfo.RenderDevice = &renderDevice;
		semaphoreCreateInfo.Name = GTSL::StaticString<32>("ImageAvailableSemaphore");
		imageAvailableSemaphore.EmplaceBack(semaphoreCreateInfo);
		semaphoreCreateInfo.Name = GTSL::StaticString<32>("RenderFinishedSemaphore");
		renderFinishedSemaphore.EmplaceBack(semaphoreCreateInfo);

		Fence::CreateInfo fence_create_info;
		fence_create_info.RenderDevice = &renderDevice;
		fence_create_info.Name = GTSL::StaticString<32>("InFlightFence");
		fence_create_info.IsSignaled = true;
		graphicsFences.EmplaceBack(fence_create_info);
		fence_create_info.Name = GTSL::StaticString<32>("TransferFence");
		fence_create_info.IsSignaled = true;
		transferFences.EmplaceBack(fence_create_info);

		{
			CommandPool::CreateInfo commandPoolCreateInfo;
			commandPoolCreateInfo.RenderDevice = &renderDevice;
			if constexpr (_DEBUG) {
				GTSL::StaticString<64> commandPoolName("Transfer command pool. Frame: "); commandPoolName += i;
				commandPoolCreateInfo.Name = commandPoolName;
			}
			commandPoolCreateInfo.Queue = &graphicsQueue;
			graphicsCommandPools.EmplaceBack(commandPoolCreateInfo);

			CommandPool::AllocateCommandBuffersInfo allocateCommandBuffersInfo;
			allocateCommandBuffersInfo.IsPrimary = true;
			allocateCommandBuffersInfo.RenderDevice = &renderDevice;

			CommandBuffer::CreateInfo commandBufferCreateInfo;
			commandBufferCreateInfo.RenderDevice = &renderDevice;
			if constexpr (_DEBUG) {
				GTSL::StaticString<64> commandBufferName("Graphics command buffer. Frame: "); commandBufferName += i;
				commandBufferCreateInfo.Name = commandBufferName;
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
			if constexpr (_DEBUG) {
				GTSL::StaticString<64> commandPoolName("Transfer command pool. Frame: "); commandPoolName += i;
				commandPoolCreateInfo.Name = commandPoolName;
			}
			commandPoolCreateInfo.Queue = &transferQueue;
			transferCommandPools.EmplaceBack(commandPoolCreateInfo);

			CommandPool::AllocateCommandBuffersInfo allocate_command_buffers_info;
			allocate_command_buffers_info.RenderDevice = &renderDevice;
			allocate_command_buffers_info.IsPrimary = true;

			CommandBuffer::CreateInfo commandBufferCreateInfo;
			commandBufferCreateInfo.RenderDevice = &renderDevice;
			if constexpr (_DEBUG) {
				GTSL::StaticString<64> commandBufferName("Transfer command buffer. Frame: "); commandBufferName += i;
				commandBufferCreateInfo.Name = commandBufferName;	
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
			if constexpr (_DEBUG) {
				GTSL::StaticString<32> name("Pipeline cache. Thread: "); name += i;
				pipelineCacheCreateInfo.Name = name;
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
			if constexpr (_DEBUG) {
				GTSL::StaticString<32> name("Pipeline cache. Thread: "); name += i;
				pipelineCacheCreateInfo.Name = name;
			}
			
			pipelineCaches.EmplaceBack(pipelineCacheCreateInfo);
		}
	}
	
	BE_LOG_MESSAGE("Initialized successfully");
}

const PipelineCache* RenderSystem::GetPipelineCache() const { return &pipelineCaches[GTSL::Thread::ThisTreadID()]; }

ComponentReference RenderSystem::CreateRayTracedMesh(const CreateRayTracingMeshInfo& info)
{
	auto& sharedMesh = sharedMeshes[info.SharedMesh()];
	
	const auto component = rayTracingMeshes.GetFirstFreeIndex().Get();

	auto verticesSize = info.VertexCount * info.VertexSize; auto indecesSize = info.IndexCount * info.IndexSize;
	auto meshSize = GTSL::Math::RoundUpByPowerOf2(verticesSize, info.IndexSize) + indecesSize;
	
	RayTracingMesh rayTracingMesh;

	//Buffer verticesBuffer, indecesBuffer;
	//HostRenderAllocation vertices, indeces;
	
	//{
	//	Buffer::CreateInfo createInfo;
	//	createInfo.RenderDevice = GetRenderDevice();
	//
	//	if constexpr (_DEBUG) {
	//		GTSL::StaticString<64> name("Render System. RayTraced Mesh Buffer");
	//		createInfo.Name = name;
	//	}
	//
	//	createInfo.Size = meshSize;
	//	createInfo.BufferType = BufferType::TRANSFER_DESTINATION | BufferType::BUILD_INPUT_READ_ONLY | BufferType::ADDRESS;
	//
	//	BufferLocalMemoryAllocationInfo bufferLocal;
	//	bufferLocal.Buffer = &rayTracingMesh.MeshBuffer;
	//	bufferLocal.Allocation = &rayTracingMesh.MeshBufferAllocation;
	//	bufferLocal.CreateInfo = &createInfo;
	//	AllocateLocalBufferMemory(bufferLocal);
	//}

	//{
	//	Buffer::CreateInfo createInfo;
	//	createInfo.RenderDevice = GetRenderDevice();
	//
	//	if constexpr (_DEBUG) {
	//		GTSL::StaticString<64> name("Render System. TestBuffer");
	//		createInfo.Name = name;
	//	}
	//
	//	createInfo.Size = verticesSize;
	//	createInfo.BufferType = BufferType::TRANSFER_DESTINATION | BufferType::BUILD_INPUT_READ_ONLY | BufferType::ADDRESS;
	//
	//	BufferScratchMemoryAllocationInfo bufferLocal;
	//	bufferLocal.Buffer = &verticesBuffer;
	//	bufferLocal.Allocation = &vertices;
	//	bufferLocal.CreateInfo = &createInfo;
	//	AllocateScratchBufferMemory(bufferLocal);
	//	
	//	createInfo.Size = indecesSize;
	//
	//	bufferLocal.Buffer = &indecesBuffer;
	//	bufferLocal.Allocation = &indeces;
	//	bufferLocal.CreateInfo = &createInfo;
	//	AllocateScratchBufferMemory(bufferLocal);
	//}
	//
	//GTSL::MemCopy(verticesSize, sharedMesh.Allocation.Data, vertices.Data);
	//GTSL::MemCopy(indecesSize, static_cast<byte*>(sharedMesh.Allocation.Data) + verticesSize, indeces.Data);
	
	//rayTracingMesh.MeshBuffer = sharedMesh.Buffer; rayTracingMesh.MeshBufferAllocation = sharedMesh.Allocation;
	
	rayTracingMesh.IndexType = SelectIndexType(info.IndexSize);
	rayTracingMesh.IndicesCount = info.IndexCount;

	auto meshDataBuffer = sharedMesh.Buffer;
	
	{
		AccelerationStructure::GeometryTriangles geometryTriangles;
		geometryTriangles.IndexType = rayTracingMesh.IndexType;
		geometryTriangles.VertexFormat = ShaderDataType::FLOAT3;
		geometryTriangles.MaxVertices = info.VertexCount;
		geometryTriangles.TransformData = 0;
		geometryTriangles.VertexData = meshDataBuffer.GetAddress(GetRenderDevice());
		geometryTriangles.IndexData = meshDataBuffer.GetAddress(GetRenderDevice()) + verticesSize;
		geometryTriangles.VertexFormat = ShaderDataType::FLOAT3;
		geometryTriangles.VertexStride = info.VertexSize;
		geometryTriangles.FirstVertex = 0;
		
		AccelerationStructure::Geometry geometry;
		geometry.Flags = 0;
		geometry.SetGeometryTriangles(geometryTriangles);
		geometry.PrimitiveCount = rayTracingMesh.IndicesCount / 3;
		geometry.PrimitiveOffset = 0;
		geometries.EmplaceBack(geometry);

		AccelerationStructure::CreateInfo accelerationStructureCreateInfo;
		accelerationStructureCreateInfo.RenderDevice = GetRenderDevice();
		if constexpr (_DEBUG) { accelerationStructureCreateInfo.Name = GTSL::StaticString<64>("Render Device. Bottom Acceleration Structure"); }
		accelerationStructureCreateInfo.Geometries = GTSL::Range<AccelerationStructure::Geometry*>(1, &geometry);
		accelerationStructureCreateInfo.DeviceAddress = 0;
		accelerationStructureCreateInfo.Offset = 0;
		
		AllocateAccelerationStructureMemory(&rayTracingMesh.AccelerationStructure, &rayTracingMesh.StructureBuffer, 
			GTSL::Range<const AccelerationStructure::Geometry*>(1, &geometry), &accelerationStructureCreateInfo,
			&rayTracingMesh.StructureBufferAllocation, BuildType::GPU_LOCAL);
	}

	rayTracingMeshes.Emplace(rayTracingMesh);

	//{
	//	BufferCopyData bufferCopyData;
	//	bufferCopyData.SourceOffset = 0;
	//	bufferCopyData.DestinationOffset = 0;
	//	bufferCopyData.SourceBuffer = sharedMesh.Buffer;
	//	bufferCopyData.DestinationBuffer = rayTracingMesh.MeshBuffer;
	//	bufferCopyData.Size = meshSize;
	//	bufferCopyData.Allocation = sharedMesh.Allocation;
	//	AddBufferCopy(bufferCopyData);
	//}
	
	
	{
		AccelerationStructureBuildData buildData;
		buildData.ScratchBuildSize = 250000;
		buildData.Destination = rayTracingMesh.AccelerationStructure;
		buildDatas.EmplaceBack(buildData);
	}

	{
		auto& instance = *(static_cast<AccelerationStructure::Instance*>(instancesAllocation.Data) + instanceCount);
		
		instance.Flags = GeometryInstanceFlags::DISABLE_CULLING | GeometryInstanceFlags::OPAQUE;
		instance.AccelerationStructureReference = rayTracingMesh.AccelerationStructure.GetAddress(GetRenderDevice());
		instance.Mask = 0xFF;
		instance.InstanceCustomIndex = component;
		instance.InstanceShaderBindingTableRecordOffset = 0;
		instance.Transform = *info.Matrix;

		BE_ASSERT(instanceCount < MAX_INSTANCES_COUNT);
		
		++instanceCount;
	}
	
	return ComponentReference(GetSystemId(), component);
}

RenderSystem::SharedMeshHandle RenderSystem::CreateSharedMesh(Id name, uint32 vertexCount, uint32 vertexSize, const uint32 indexCount, const uint32 indexSize)
{
	SharedMesh mesh;

	Buffer::CreateInfo createInfo;
	createInfo.RenderDevice = GetRenderDevice();
	createInfo.BufferType = BufferType::VERTEX | BufferType::INDEX | BufferType::ADDRESS | BufferType::TRANSFER_SOURCE | BufferType::BUILD_INPUT_READ_ONLY;
	createInfo.Size = vertexCount * vertexSize + indexCount * indexSize;
	mesh.Size = createInfo.Size;

	mesh.IndexType = SelectIndexType(indexSize);
	mesh.IndicesCount = indexCount;

	BufferScratchMemoryAllocationInfo bufferLocal;
	bufferLocal.CreateInfo = &createInfo;
	bufferLocal.Allocation = &mesh.Allocation;
	bufferLocal.Buffer = &mesh.Buffer;
	AllocateScratchBufferMemory(bufferLocal);

	mesh.OffsetToIndices = vertexCount * vertexSize; //TODO: MAYBE ROUND TO INDEX SIZE?
	
	auto place = sharedMeshes.Emplace(mesh);

	return SharedMeshHandle(place);
}

RenderSystem::GPUMeshHandle RenderSystem::CreateGPUMesh(SharedMeshHandle sharedMeshHandle)
{
	auto& sharedMesh = sharedMeshes[sharedMeshHandle()];

	GPUMesh mesh;
	
	Buffer::CreateInfo createInfo;
	createInfo.RenderDevice = GetRenderDevice();
	createInfo.BufferType = BufferType::VERTEX | BufferType::INDEX | BufferType::ADDRESS | BufferType::TRANSFER_DESTINATION;
	createInfo.Size = sharedMesh.Size;

	mesh.IndexType = sharedMesh.IndexType;
	mesh.IndicesCount = sharedMesh.IndicesCount;
	mesh.OffsetToIndices = sharedMesh.OffsetToIndices;
	
	BufferLocalMemoryAllocationInfo bufferLocal;
	bufferLocal.CreateInfo = &createInfo;
	bufferLocal.Allocation = &mesh.Allocation;
	bufferLocal.Buffer = &mesh.Buffer;
	AllocateLocalBufferMemory(bufferLocal);

	BufferCopyData bufferCopyData;
	bufferCopyData.Size = sharedMesh.Size;
	bufferCopyData.DestinationBuffer = mesh.Buffer;
	bufferCopyData.DestinationOffset = 0;
	bufferCopyData.SourceBuffer = sharedMesh.Buffer;
	bufferCopyData.SourceOffset = 0;
	AddBufferCopy(bufferCopyData);

	return GPUMeshHandle(gpuMeshes.Emplace(mesh));
}

void RenderSystem::RenderMesh(GPUMeshHandle handle, const uint32 instanceCount)
{
	auto& mesh = gpuMeshes[handle()];

	{
		CommandBuffer::BindVertexBufferInfo bindInfo;
		bindInfo.RenderDevice = GetRenderDevice();
		bindInfo.Buffer = mesh.Buffer;
		bindInfo.Offset = 0;
		graphicsCommandBuffers[GetCurrentFrame()].BindVertexBuffer(bindInfo);
	}

	{
		CommandBuffer::BindIndexBufferInfo bindInfo;
		bindInfo.RenderDevice = GetRenderDevice();
		bindInfo.Buffer = mesh.Buffer;
		bindInfo.Offset = mesh.OffsetToIndices;
		bindInfo.IndexType = mesh.IndexType;
		graphicsCommandBuffers[GetCurrentFrame()].BindIndexBuffer(bindInfo);
	}
	
	CommandBuffer::DrawIndexedInfo drawIndexedInfo;
	drawIndexedInfo.RenderDevice = GetRenderDevice();
	drawIndexedInfo.InstanceCount = instanceCount;
	drawIndexedInfo.IndexCount = mesh.IndicesCount;
	graphicsCommandBuffers[GetCurrentFrame()].DrawIndexed(drawIndexedInfo);
}

void RenderSystem::RenderAllMeshesForMaterial(Id material)
{
	auto range = meshesByMaterial.At(material()).GetRange(); //DECLARE MATS FROM MAT SYSTEM, THEN ADD MESHES OR ELSE IT BLOWS UP BECAUSE NO MATERIALS ARE AVAILABLE
	
	for(auto& e : range)
	{
		auto& mesh = gpuMeshes[e];

		{
			CommandBuffer::BindVertexBufferInfo bindInfo;
			bindInfo.RenderDevice = GetRenderDevice();
			bindInfo.Buffer = mesh.Buffer;
			bindInfo.Offset = 0;
			graphicsCommandBuffers[GetCurrentFrame()].BindVertexBuffer(bindInfo);
		}

		{
			CommandBuffer::BindIndexBufferInfo bindInfo;
			bindInfo.RenderDevice = GetRenderDevice();
			bindInfo.Buffer = mesh.Buffer;
			bindInfo.Offset = mesh.OffsetToIndices;
			bindInfo.IndexType = mesh.IndexType;
			graphicsCommandBuffers[GetCurrentFrame()].BindIndexBuffer(bindInfo);
		}

		{
			CommandBuffer::DrawIndexedInfo drawIndexedInfo;
			drawIndexedInfo.RenderDevice = GetRenderDevice();
			drawIndexedInfo.InstanceCount = 1;
			drawIndexedInfo.IndexCount = mesh.IndicesCount;
			graphicsCommandBuffers[GetCurrentFrame()].DrawIndexed(drawIndexedInfo);
		}
	}
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
		recreate.Name = name;
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
				textureViewCreateInfos[i].Name = name;
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

	meshesByMaterial.Initialize(32, GetPersistentAllocator());
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

void RenderSystem::buildAccelerationStructuresOnDevice(CommandBuffer& commandBuffer)
{
	if (buildDatas.GetLength())
	{
		GTSL::Array<GAL::BuildAccelerationStructureInfo, 8> accelerationStructureBuildInfos;
		GTSL::Array<GTSL::Array<AccelerationStructure::Geometry, 8>, 16> geometryDescriptors;

		uint32 offset = 0;
		
		for (uint32 i = 0; i < buildDatas.GetLength(); ++i)
		{
			geometryDescriptors.EmplaceBack();
			geometryDescriptors[i].EmplaceBack(geometries[i]);
			
			GAL::BuildAccelerationStructureInfo buildAccelerationStructureInfo;
			buildAccelerationStructureInfo.ScratchBufferAddress = scratchBufferAddress + offset; //TODO: ENSURE CURRENT BUILDS SCRATCH BUFFER AREN'T OVERWRITTEN ON TURN OF FRAME
			buildAccelerationStructureInfo.SourceAccelerationStructure = AccelerationStructure();
			buildAccelerationStructureInfo.DestinationAccelerationStructure = buildDatas[i].Destination;
			buildAccelerationStructureInfo.Geometries = geometryDescriptors[i];
			buildAccelerationStructureInfo.Flags = buildDatas[i].BuildFlags;

			accelerationStructureBuildInfos.EmplaceBack(buildAccelerationStructureInfo);
			
			offset += GTSL::Math::RoundUpByPowerOf2(buildDatas[i].ScratchBuildSize, scratchBufferOffsetAlignment);
		}

		GAL::BuildAccelerationStructuresInfo build;
		build.RenderDevice = GetRenderDevice();
		build.BuildAccelerationStructureInfos = accelerationStructureBuildInfos;
		
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
	
	buildDatas.ResizeDown(0);
	geometries.ResizeDown(0);
}

void RenderSystem::renderBegin(TaskInfo taskInfo)
{	
	auto& commandBuffer = graphicsCommandBuffers[currentFrameIndex];
	
	commandBuffer.BeginRecording({});
}

void RenderSystem::renderFinish(TaskInfo taskInfo)
{
	auto& commandBuffer = graphicsCommandBuffers[currentFrameIndex];

	AccelerationStructure::Geometry geometry;
	geometry.Flags = 0;
	geometry.PrimitiveCount = instanceCount;
	geometry.PrimitiveOffset = 0;
	geometry.SetGeometryInstances(AccelerationStructure::GeometryInstances{ instancesBuffer.GetAddress(GetRenderDevice()) });
	geometries.EmplaceBack(geometry);
	
	AccelerationStructureBuildData buildData;
	buildData.BuildFlags = 0;
	buildData.Destination = topLevelAccelerationStructure;
	buildData.ScratchBuildSize = 250000;
	buildDatas.EmplaceBack(buildData);
	
	buildAccelerationStructures(this, commandBuffer);
	
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
	transferFences[GetCurrentFrame()].Wait(GetRenderDevice());

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
		//triangleDatas.Pop(0, processedAccelerationStructureBuilds[GetCurrentFrame()]);

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
			copy_buffers_info.Destination = e.DestinationBuffer;
			copy_buffers_info.DestinationOffset = e.DestinationOffset;
			copy_buffers_info.Source = e.SourceBuffer;
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
		pipelineBarrierInfo.InitialStage = PipelineStage::TRANSFER;
		pipelineBarrierInfo.FinalStage = PipelineStage::TRANSFER;
		commandBuffer.AddPipelineBarrier(pipelineBarrierInfo);

		for (uint32 i = 0; i < textureCopyData.GetLength(); ++i)
		{
			CommandBuffer::CopyBufferToTextureInfo copyBufferToImageInfo;
			copyBufferToImageInfo.RenderDevice = GetRenderDevice();
			copyBufferToImageInfo.DestinationTexture = textureCopyData[i].DestinationTexture;
			copyBufferToImageInfo.Offset = { 0, 0, 0 };
			copyBufferToImageInfo.Extent = textureCopyData[i].Extent;
			copyBufferToImageInfo.SourceBuffer = textureCopyData[i].SourceBuffer;
			copyBufferToImageInfo.TextureLayout = textureCopyData[i].Layout;
			commandBuffer.CopyBufferToTexture(copyBufferToImageInfo);
		}
			
		pipelineBarrierInfo.TextureBarriers = destinationTextureBarriers;
		pipelineBarrierInfo.InitialStage = PipelineStage::TRANSFER;
		pipelineBarrierInfo.FinalStage = PipelineStage::FRAGMENT_SHADER | PipelineStage::RAY_TRACING_SHADER;
		commandBuffer.AddPipelineBarrier(pipelineBarrierInfo);

		processedTextureCopies[GetCurrentFrame()] = textureCopyData.GetLength();
	}
	
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
