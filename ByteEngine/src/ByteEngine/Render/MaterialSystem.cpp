#include "MaterialSystem.h"

#include "FrameManager.h"
#include "RenderSystem.h"
#include "ByteEngine/Resources/TextureResourceManager.h"

const char* BindingTypeString(const BindingType binding)
{
	switch (binding)
	{
	case BindingType::UNIFORM_BUFFER_DYNAMIC: return "UNIFORM_BUFFER_DYNAMIC";
	case BindingType::COMBINED_IMAGE_SAMPLER: return "COMBINED_IMAGE_SAMPLER";
	case BindingType::UNIFORM_BUFFER: return "UNIFORM_BUFFER";
	default: return "null";
	}
}

void MaterialSystem::Initialize(const InitializeInfo& initializeInfo)
{
	renderGroups.Initialize(32, GetPersistentAllocator());
	materials.Initialize(32, GetPersistentAllocator());

	//auto* renderSystem = initializeInfo.GameInstance->GetSystem<RenderSystem>("RenderSystem");
	minUniformBufferOffset = 64;//renderSystem->GetRenderDevice()->GetMinUniformBufferOffset(); //TODO: FIX!!!
	
	{
		const GTSL::Array<TaskDependency, 6> taskDependencies{ { "MaterialSystem", AccessType::READ_WRITE }, { "RenderSystem", AccessType::READ } };
		initializeInfo.GameInstance->AddTask("updateDescriptors", GTSL::Delegate<void(TaskInfo)>::Create<MaterialSystem, &MaterialSystem::updateDescriptors>(this), taskDependencies, "FrameStart", "RenderStart");
		initializeInfo.GameInstance->AddTask("updateDescriptors", GTSL::Delegate<void(TaskInfo)>::Create<MaterialSystem, &MaterialSystem::updateDescriptors>(this), taskDependencies, "RenderStart", "RenderSetup");
	}
	
	{
		const GTSL::Array<TaskDependency, 6> taskDependencies{ { "MaterialSystem", AccessType::READ_WRITE }, };
		initializeInfo.GameInstance->AddTask("updateCounter", GTSL::Delegate<void(TaskInfo)>::Create<MaterialSystem, &MaterialSystem::updateCounter>(this), taskDependencies, "RenderEnd", "FrameEnd");
	}

	isRenderGroupReady.Initialize(32, GetPersistentAllocator());
	isMaterialReady.Initialize(32, GetPersistentAllocator());

	textures.Initialize(64, GetPersistentAllocator());
	
	perFrameBindingsUpdateData.Resize(MAX_CONCURRENT_FRAMES);
	for(auto& e : perFrameBindingsUpdateData)
	{
		e.Initialize(32, GetPersistentAllocator());
	}
	
	frame = 0;
}

void MaterialSystem::Shutdown(const ShutdownInfo& shutdownInfo)
{
	RenderSystem* renderSystem = shutdownInfo.GameInstance->GetSystem<RenderSystem>("RenderSystem");

	GTSL::ForEach(renderGroups,	[&](RenderGroupData& renderGroup)
	{
		renderGroup.BindingsPool.Destroy(renderSystem->GetRenderDevice());
		renderGroup.BindingsSetLayout.Destroy(renderSystem->GetRenderDevice());
	});

	GTSL::ForEach(materials, [&](MaterialInstance& e)
	{
		e.Pipeline.Destroy(renderSystem->GetRenderDevice());
		e.BindingsPool.Destroy(renderSystem->GetRenderDevice());
		e.BindingsSetLayout.Destroy(renderSystem->GetRenderDevice());
	});
}

void MaterialSystem::SetGlobalState(GameInstance* gameInstance, const GTSL::Array<GTSL::Array<BindingType, 6>, 6>& globalState)
{
	RenderSystem* renderSystem = gameInstance->GetSystem<RenderSystem>("RenderSystem");

	BE_ASSERT(globalState[0].GetLength() == 1 && globalState.GetLength() == 1, "Only one binding set is supported");
	
	for(uint32 i = 0; i < globalState.GetLength(); ++i)
	{
		BindingsSetLayout::CreateInfo bindingsSetLayoutCreateInfo;
		bindingsSetLayoutCreateInfo.RenderDevice = renderSystem->GetRenderDevice();

		GTSL::Array<BindingsSetLayout::BindingDescriptor, 10> bindingDescriptors;
		for(uint32 j = 0; j < globalState[i].GetLength(); ++j)
		{
			bindingDescriptors.PushBack(BindingsSetLayout::BindingDescriptor{ globalState[i][j], ShaderStage::ALL, 25/*max bindings, TODO: CHECK HOW TO UPDATE*/, BindingFlags::PARTIALLY_BOUND | BindingFlags::VARIABLE_DESCRIPTOR_COUNT });
		}

		if constexpr (_DEBUG)
		{
			GTSL::StaticString<128> name("Bindings set layout. Material system global state");
			bindingsSetLayoutCreateInfo.Name = name.begin();
		}
		
		bindingsSetLayoutCreateInfo.BindingsDescriptors = bindingDescriptors;
		globalBindingsSetLayout.EmplaceBack(bindingsSetLayoutCreateInfo);
	}

	BindingsPool::CreateInfo bindingsPoolCreateInfo;
	bindingsPoolCreateInfo.RenderDevice = renderSystem->GetRenderDevice();

	if constexpr (_DEBUG)
	{
		GTSL::StaticString<64> name("Bindings pool. Global state");
		bindingsPoolCreateInfo.Name = name.begin();
	}
	
	GTSL::Array<BindingsPool::DescriptorPoolSize, 10> descriptor_pool_sizes;
	descriptor_pool_sizes.PushBack(BindingsPool::DescriptorPoolSize{ BindingType::UNIFORM_BUFFER_DYNAMIC, 6 });
	descriptor_pool_sizes.PushBack(BindingsPool::DescriptorPoolSize{ BindingType::COMBINED_IMAGE_SAMPLER, 16 });
	descriptor_pool_sizes.PushBack(BindingsPool::DescriptorPoolSize{ BindingType::STORAGE_BUFFER_DYNAMIC, 16 });
	bindingsPoolCreateInfo.DescriptorPoolSizes = descriptor_pool_sizes;
	bindingsPoolCreateInfo.MaxSets = MAX_CONCURRENT_FRAMES;
	globalBindingsPool = BindingsPool(bindingsPoolCreateInfo);

	{
		BindingsPool::AllocateBindingsSetsInfo allocateBindingsSetsInfo;
		allocateBindingsSetsInfo.RenderDevice = renderSystem->GetRenderDevice();
		allocateBindingsSetsInfo.BindingsSets = GTSL::Ranger<BindingsSet>(MAX_CONCURRENT_FRAMES, globalBindingsSets.begin());
		GTSL::Array<BindingsSetLayout, 6 * MAX_CONCURRENT_FRAMES> bindingsSetLayouts;
		for (uint32 i = 0; i < globalState.GetLength(); ++i)
		{
			for (uint32 j = 0; j < MAX_CONCURRENT_FRAMES; ++j)
			{
				bindingsSetLayouts.EmplaceBack(globalBindingsSetLayout[i]);
			}
		}
		allocateBindingsSetsInfo.BindingsSetLayouts = bindingsSetLayouts;
		allocateBindingsSetsInfo.BindingsSetDynamicBindingsCounts = GTSL::Array<uint32, 2>{ 2 };

		{
			GTSL::Array<GAL::VulkanCreateInfo, MAX_CONCURRENT_FRAMES> bindingsSetsCreateInfo(MAX_CONCURRENT_FRAMES);

			if constexpr (_DEBUG)
			{
				for (uint32 j = 0; j < MAX_CONCURRENT_FRAMES; ++j)
				{
					GTSL::StaticString<64> name("BindingsSet. Global state");
					bindingsSetsCreateInfo[j].RenderDevice = renderSystem->GetRenderDevice();
					bindingsSetsCreateInfo[j].Name = name.begin();
				}
			}
			
			allocateBindingsSetsInfo.BindingsSetCreateInfos = bindingsSetsCreateInfo;
		}
		
		globalBindingsPool.AllocateBindingsSets(allocateBindingsSetsInfo);
		
		globalBindingsSets.Resize(MAX_CONCURRENT_FRAMES);
	}
	
	{
		PipelineLayout::CreateInfo pipelineLayout;
		pipelineLayout.RenderDevice = renderSystem->GetRenderDevice();
		
		if constexpr (_DEBUG)
		{
			GTSL::StaticString<128> name("Pipeline Layout. Material system global state");
			pipelineLayout.Name = name.begin();
		}

		pipelineLayout.BindingsSetLayouts = globalBindingsSetLayout;
		globalPipelineLayout.Initialize(pipelineLayout);
	}
}

void MaterialSystem::AddRenderGroup(GameInstance* gameInstance, const AddRenderGroupInfo& addRenderGroupInfo)
{
	RenderGroupData& renderGroupData = renderGroups.Emplace(addRenderGroupInfo.Name);

	RenderSystem* renderSystem = gameInstance->GetSystem<RenderSystem>("RenderSystem");

	BE_ASSERT(addRenderGroupInfo.Bindings.GetLength() < 2, "Only one binding set is supported");

	for (auto& e : perFrameBindingsUpdateData)
	{
		auto& updateData = e.RenderGroups.Emplace(addRenderGroupInfo.Name);

		updateData.BufferBindingDescriptorsUpdates.Initialize(2, GetPersistentAllocator());
		updateData.TextureBindingDescriptorsUpdates.Initialize(2, GetPersistentAllocator());
		updateData.BufferBindingTypes.Initialize(2, GetPersistentAllocator());
	}
	
	for (uint32 i = 0; i < addRenderGroupInfo.Bindings.GetLength(); ++i)
	{
		BindingsSetLayout::CreateInfo setLayout;
		setLayout.RenderDevice = renderSystem->GetRenderDevice();

		if constexpr (_DEBUG)
		{
			GTSL::StaticString<64> name("Bindings set layout. Render group: "); name += addRenderGroupInfo.Name;
			setLayout.Name = name.begin();
		}

		GTSL::Array<BindingsSetLayout::BindingDescriptor, 10> bindingDescriptors;
		for (uint32 j = 0; j < addRenderGroupInfo.Bindings[i].GetLength(); ++j)
		{
			bindingDescriptors.PushBack(BindingsSetLayout::BindingDescriptor{ addRenderGroupInfo.Bindings[i][j], ShaderStage::ALL, 1, 0 });
		}
		
		setLayout.BindingsDescriptors = bindingDescriptors;

		renderGroupData.BindingsSetLayout = BindingsSetLayout(setLayout);
	}
	//Bindings set layout

	{
		BindingsPool::CreateInfo bindingsPoolCreateInfo;
		bindingsPoolCreateInfo.RenderDevice = renderSystem->GetRenderDevice();

		if constexpr (_DEBUG)
		{
			GTSL::StaticString<64> name("Bindings pool. Render group: "); name += addRenderGroupInfo.Name;
			bindingsPoolCreateInfo.Name = name.begin();
		}

		GTSL::Array<BindingsPool::DescriptorPoolSize, 10> descriptorPoolSizes;
		descriptorPoolSizes.PushBack(BindingsPool::DescriptorPoolSize{ BindingType::UNIFORM_BUFFER_DYNAMIC, 6 });
		descriptorPoolSizes.PushBack(BindingsPool::DescriptorPoolSize{ BindingType::UNIFORM_BUFFER, 6 });
		descriptorPoolSizes.PushBack(BindingsPool::DescriptorPoolSize{ BindingType::COMBINED_IMAGE_SAMPLER, 16 });
		descriptorPoolSizes.PushBack(BindingsPool::DescriptorPoolSize{ BindingType::STORAGE_BUFFER_DYNAMIC, 16 });
		bindingsPoolCreateInfo.DescriptorPoolSizes = descriptorPoolSizes;
		bindingsPoolCreateInfo.MaxSets = MAX_CONCURRENT_FRAMES;
		renderGroupData.BindingsPool = BindingsPool(bindingsPoolCreateInfo);
	}
	//Bindings pool

	{
		BindingsPool::AllocateBindingsSetsInfo allocateBindings;
		allocateBindings.RenderDevice = renderSystem->GetRenderDevice();
		allocateBindings.BindingsSets = GTSL::Ranger<BindingsSet>(MAX_CONCURRENT_FRAMES, renderGroupData.BindingsSets.begin());
		{
			GTSL::Array<BindingsSetLayout, 6 * MAX_CONCURRENT_FRAMES> bindingsSetLayouts;
			for (uint32 i = 0; i < addRenderGroupInfo.Bindings.GetLength(); ++i)
			{
				for (uint32 j = 0; j < MAX_CONCURRENT_FRAMES; ++j)
				{
					bindingsSetLayouts.EmplaceBack(renderGroupData.BindingsSetLayout);
				}
			}

			allocateBindings.BindingsSetLayouts = bindingsSetLayouts;
			allocateBindings.BindingsSetDynamicBindingsCounts = GTSL::Array<uint32, 2>{ 1 };

			{
				GTSL::Array<GAL::VulkanCreateInfo, MAX_CONCURRENT_FRAMES> bindingsSetsCreateInfo(MAX_CONCURRENT_FRAMES);

				if constexpr (_DEBUG)
				{
					for (uint32 j = 0; j < MAX_CONCURRENT_FRAMES; ++j)
					{
						GTSL::StaticString<64> name("BindingsSet. Render Group: "); name += addRenderGroupInfo.Name;
						bindingsSetsCreateInfo[j].RenderDevice = renderSystem->GetRenderDevice();
						bindingsSetsCreateInfo[j].Name = name.begin();
					}
				}

				allocateBindings.BindingsSetCreateInfos = bindingsSetsCreateInfo;
			}

			renderGroupData.BindingsPool.AllocateBindingsSets(allocateBindings);

			renderGroupData.BindingsSets.Resize(MAX_CONCURRENT_FRAMES);
		}
	}

	{
		GTSL::Array<BindingsSetLayout, 16> bindingsSetLayouts;
		bindingsSetLayouts.EmplaceBack(globalBindingsSetLayout[0]); //global bindings
		bindingsSetLayouts.EmplaceBack(renderGroupData.BindingsSetLayout); //render group bindings

		PipelineLayout::CreateInfo pipelineLayout;
		pipelineLayout.RenderDevice = renderSystem->GetRenderDevice();

		if constexpr (_DEBUG)
		{
			GTSL::StaticString<128> name("Pipeline layout. Render group: "); name += addRenderGroupInfo.Name;
			pipelineLayout.Name = name.begin();
		}

		pipelineLayout.BindingsSetLayouts = bindingsSetLayouts;
		renderGroupData.PipelineLayout.Initialize(pipelineLayout);
	}

	for (uint32 i = 0; i < addRenderGroupInfo.Bindings.GetLength(); ++i)
	{
		BindingsSet::BindingsSetUpdateInfo bindingsSetUpdateInfo;
		bindingsSetUpdateInfo.RenderDevice = renderSystem->GetRenderDevice();

		for (uint32 j = 0; j < addRenderGroupInfo.Bindings[i].GetLength(); ++j)
		{
			if (addRenderGroupInfo.Bindings[i][j] == GAL::VulkanBindingType::UNIFORM_BUFFER_DYNAMIC)
			{
				Buffer::CreateInfo bufferInfo;
				bufferInfo.RenderDevice = renderSystem->GetRenderDevice();

				if constexpr (_DEBUG)
				{
					GTSL::StaticString<64> name("Uniform Buffer. Render group: "); name += addRenderGroupInfo.Name;
					bufferInfo.Name = name.begin();
				}

				bufferInfo.Size = addRenderGroupInfo.Size[i][j];
				bufferInfo.BufferType = BufferType::UNIFORM;
				renderGroupData.Buffer = Buffer(bufferInfo);

				RenderSystem::BufferScratchMemoryAllocationInfo memoryAllocationInfo;
				memoryAllocationInfo.Buffer = renderGroupData.Buffer;
				memoryAllocationInfo.Allocation = &renderGroupData.Allocation;
				renderSystem->AllocateScratchBufferMemory(memoryAllocationInfo);

				renderGroupData.BindingType = BindingType::UNIFORM_BUFFER_DYNAMIC;

				for (auto& e : perFrameBindingsUpdateData)
				{
					BindingsSet::BufferBindingsUpdateInfo bufferBindingsUpdateInfo;
					bufferBindingsUpdateInfo.Buffer = renderGroupData.Buffer;
					bufferBindingsUpdateInfo.Offset = 0;
					bufferBindingsUpdateInfo.Range = addRenderGroupInfo.Range[i][j];

					e.RenderGroups.At(addRenderGroupInfo.Name).BufferBindingDescriptorsUpdates.EmplaceBack(bufferBindingsUpdateInfo);
					e.RenderGroups.At(addRenderGroupInfo.Name).BufferBindingTypes.EmplaceBack(renderGroupData.BindingType);
				}
			}

			if (addRenderGroupInfo.Bindings[i][j] == GAL::VulkanBindingType::STORAGE_BUFFER_DYNAMIC)
			{
				Buffer::CreateInfo bufferInfo;
				bufferInfo.RenderDevice = renderSystem->GetRenderDevice();

				if constexpr (_DEBUG)
				{
					GTSL::StaticString<64> name("Storage buffer. Render group: "); name += addRenderGroupInfo.Name;
					bufferInfo.Name = name.begin();
				}

				bufferInfo.Size = addRenderGroupInfo.Size[i][j];
				bufferInfo.BufferType = BufferType::STORAGE;
				renderGroupData.Buffer = Buffer(bufferInfo);

				RenderSystem::BufferScratchMemoryAllocationInfo memoryAllocationInfo;
				memoryAllocationInfo.Buffer = renderGroupData.Buffer;
				memoryAllocationInfo.Allocation = &renderGroupData.Allocation;
				renderSystem->AllocateScratchBufferMemory(memoryAllocationInfo);

				renderGroupData.BindingType = BindingType::STORAGE_BUFFER_DYNAMIC;
				
				for (auto& e : perFrameBindingsUpdateData)
				{
					BindingsSet::BufferBindingsUpdateInfo bufferBindingsUpdateInfo;
					bufferBindingsUpdateInfo.Buffer = renderGroupData.Buffer;
					bufferBindingsUpdateInfo.Offset = 0;
					bufferBindingsUpdateInfo.Range = addRenderGroupInfo.Range[i][j];

					e.RenderGroups.At(addRenderGroupInfo.Name).BufferBindingDescriptorsUpdates.EmplaceBack(bufferBindingsUpdateInfo);
					e.RenderGroups.At(addRenderGroupInfo.Name).BufferBindingTypes.EmplaceBack(renderGroupData.BindingType);
				}
			}
		}
	}

	isRenderGroupReady.Emplace(addRenderGroupInfo.Name, false);
}

MaterialHandle MaterialSystem::CreateMaterial(const CreateMaterialInfo& info)
{
	uint32 material_size = 0;
	info.MaterialResourceManager->GetMaterialSize(info.MaterialName, material_size);

	GTSL::Buffer material_buffer; material_buffer.Allocate(material_size, 32, GetPersistentAllocator());
	
	const auto acts_on = GTSL::Array<TaskDependency, 16>{ { "RenderSystem", AccessType::READ_WRITE }, { "MaterialSystem", AccessType::READ_WRITE }, { "FrameManager", AccessType::READ } };
	MaterialResourceManager::MaterialLoadInfo material_load_info;
	material_load_info.ActsOn = acts_on;
	material_load_info.GameInstance = info.GameInstance;
	material_load_info.Name = info.MaterialName;
	material_load_info.DataBuffer = GTSL::Ranger<byte>(material_buffer.GetCapacity(), material_buffer.GetData());
	auto* matLoadInfo = GTSL::New<MaterialLoadInfo>(GetPersistentAllocator(), info.RenderSystem, MoveRef(material_buffer), material);
	material_load_info.UserData = DYNAMIC_TYPE(MaterialLoadInfo, matLoadInfo);
	material_load_info.OnMaterialLoad = GTSL::Delegate<void(TaskInfo, MaterialResourceManager::OnMaterialLoadInfo)>::Create<MaterialSystem, &MaterialSystem::onMaterialLoaded>(this);
	info.MaterialResourceManager->LoadMaterial(material_load_info);

	return MaterialHandle{ info.MaterialName, material++ };
}

void MaterialSystem::SetDynamicMaterialParameter(const MaterialHandle material, GAL::ShaderDataType type, Id parameterName, void* data)
{
	auto& mat = materials[material.MaterialInstance];

	auto* matData = static_cast<byte*>(mat.Allocation.Data) + mat.DataSize * material.MaterialInstance;
	
	//TODO: DEFER WRITING TO NOT OVERWRITE RUNNING FRAME
	byte* FILL = matData + mat.DynamicParameters.At(parameterName);
	GTSL::MemCopy(ShaderDataTypesSize(type), data, FILL);
	FILL += GTSL::Math::PowerOf2RoundUp(mat.DataSize, static_cast<uint32>(minUniformBufferOffset));
	GTSL::MemCopy(ShaderDataTypesSize(type), data, FILL);
}

void MaterialSystem::SetMaterialParameter(const MaterialHandle material, GAL::ShaderDataType type, Id parameterName, void* data)
{
	auto& mat = materials[material.MaterialInstance];

	auto* matData = static_cast<byte*>(mat.Allocation.Data) + mat.DataSize * material.MaterialInstance;

	//TODO: DEFER WRITING TO NOT OVERWRITE RUNNING FRAME
	//byte* FILL = matData + mat.Parameters.At(parameterName);
	GTSL::MemCopy(ShaderDataTypesSize(type), data, FILL);
	FILL += GTSL::Math::PowerOf2RoundUp(mat.DataSize, static_cast<uint32>(minUniformBufferOffset));
	GTSL::MemCopy(ShaderDataTypesSize(type), data, FILL);
}

System::ComponentReference MaterialSystem::CreateTexture(const CreateTextureInfo& info)
{
	TextureResourceManager::TextureLoadInfo textureLoadInfo;
	textureLoadInfo.GameInstance = info.GameInstance;
	textureLoadInfo.Name = info.TextureName;

	textureLoadInfo.OnTextureLoadInfo = GTSL::Delegate<void(TaskInfo, TextureResourceManager::OnTextureLoadInfo)>::Create<MaterialSystem, &MaterialSystem::onTextureLoad>(this);

	const GTSL::Array<TaskDependency, 6> loadTaskDependencies{ { "RenderSystem", AccessType::READ_WRITE }, { "MaterialSystem", AccessType::READ_WRITE } };

	textureLoadInfo.ActsOn = loadTaskDependencies;

	auto component = textures.GetFirstFreeIndex().Get();

	{
		Buffer::CreateInfo scratchBufferCreateInfo;
		scratchBufferCreateInfo.RenderDevice = info.RenderSystem->GetRenderDevice();

		if constexpr (_DEBUG)
		{
			GTSL::StaticString<64> name("Scratch Buffer. Texture: "); name += info.TextureName.GetHash();
			scratchBufferCreateInfo.Name = name.begin();
		}

		{
			uint32 textureSize; GAL::TextureFormat textureFormat; GTSL::Extent3D textureExtent;
			info.TextureResourceManager->GetTextureSizeFormatExtent(info.TextureName, &textureSize, &textureFormat, &textureExtent);

			RenderDevice::FindSupportedImageFormat findFormatInfo;
			findFormatInfo.TextureTiling = TextureTiling::OPTIMAL;
			findFormatInfo.TextureUses = TextureUses::TRANSFER_DESTINATION | TextureUses::SAMPLE;
			GTSL::Array<TextureFormat, 16> candidates; candidates.EmplaceBack(ConvertFormat(textureFormat)); candidates.EmplaceBack(TextureFormat::RGBA_I8);
			findFormatInfo.Candidates = candidates;
			const auto supportedFormat = info.RenderSystem->GetRenderDevice()->FindNearestSupportedImageFormat(findFormatInfo);

			scratchBufferCreateInfo.Size = textureExtent.Width * textureExtent.Depth * textureExtent.Height * FormatSize(supportedFormat);
		}

		scratchBufferCreateInfo.BufferType = BufferType::TRANSFER_SOURCE;

		auto scratchBuffer = Buffer(scratchBufferCreateInfo);

		HostRenderAllocation allocation;

		{
			RenderSystem::BufferScratchMemoryAllocationInfo scratchMemoryAllocation;
			scratchMemoryAllocation.Buffer = scratchBuffer;
			scratchMemoryAllocation.Allocation = &allocation;
			info.RenderSystem->AllocateScratchBufferMemory(scratchMemoryAllocation);
		}


		auto* loadInfo = GTSL::New<TextureLoadInfo>(GetPersistentAllocator(), component, scratchBuffer, info.RenderSystem, allocation);

		textureLoadInfo.DataBuffer = GTSL::Ranger<byte>(allocation.Size, static_cast<byte*>(allocation.Data));

		textureLoadInfo.UserData = DYNAMIC_TYPE(LoadInfo, loadInfo);
	}

	info.TextureResourceManager->LoadTexture(textureLoadInfo);

	return component;
}

void MaterialSystem::UpdateRenderGroupData(const UpdateRenderGroupDataInfo& updateRenderGroupDataInfo)
{
}

void MaterialSystem::updateDescriptors(TaskInfo taskInfo)
{	
	BindingsSet::BindingsSetUpdateInfo bindingsUpdateInfo;
	bindingsUpdateInfo.RenderDevice = taskInfo.GameInstance->GetSystem<RenderSystem>("RenderSystem")->GetRenderDevice();

	{
		auto& bindingsUpdate = perFrameBindingsUpdateData[frame].Global;
		
		if (bindingsUpdate.BufferBindingDescriptorsUpdates.GetLength() + bindingsUpdate.TextureBindingDescriptorsUpdates.GetLength())
		{
			auto length = bindingsUpdate.BufferBindingDescriptorsUpdates.GetLength() + bindingsUpdate.TextureBindingDescriptorsUpdates.GetLength();
			
			Vector<BindingsSet::BindingUpdateInfo, BE::TAR> bindingUpdateInfos(2/*bindings sets*/, GetTransientAllocator());
			{
				BindingsSet::BindingUpdateInfo bindingUpdateInfo;

				bindingUpdateInfo.Type = GAL::VulkanBindingType::COMBINED_IMAGE_SAMPLER;
				bindingUpdateInfo.ArrayElement = 0;
				bindingUpdateInfo.Count = bindingsUpdate.TextureBindingDescriptorsUpdates.GetLength(); //TODO: NOOOO!
				bindingUpdateInfo.BindingsUpdates = bindingsUpdate.TextureBindingDescriptorsUpdates.GetData();

				bindingUpdateInfos.EmplaceBack(bindingUpdateInfo);
			}

			bindingsUpdateInfo.BindingUpdateInfos = bindingUpdateInfos;

			globalBindingsSets[frame].Update(bindingsUpdateInfo);

			bindingsUpdate.BufferBindingDescriptorsUpdates.ResizeDown(0);
			bindingsUpdate.TextureBindingDescriptorsUpdates.ResizeDown(0);
			bindingsUpdate.BufferBindingTypes.ResizeDown(0);
		}
	}

	{
		auto& bindingsUpdate = perFrameBindingsUpdateData[frame].RenderGroups;

		GTSL::PairForEach(bindingsUpdate, [&](uint64 key, BindingsUpdateData::Updates& updates)
		{
			Vector<BindingsSet::BindingUpdateInfo, BE::TAR> bindingUpdateInfos(16, updates.BufferBindingDescriptorsUpdates.GetLength(), GetTransientAllocator());
			for (uint32 i = 0; i < bindingUpdateInfos.GetLength(); ++i)
			{
				bindingUpdateInfos[i].Type = updates.BufferBindingTypes[i];
				bindingUpdateInfos[i].ArrayElement = 0;
				bindingUpdateInfos[i].Count = updates.BufferBindingDescriptorsUpdates.GetLength();
				bindingUpdateInfos[i].BindingsUpdates = updates.BufferBindingDescriptorsUpdates.GetData();
			}

			bindingsUpdateInfo.BindingUpdateInfos = bindingUpdateInfos;

			renderGroups.At(key).BindingsSets[frame].Update(bindingsUpdateInfo);
			isRenderGroupReady.At(key) = true;

			updates.BufferBindingDescriptorsUpdates.ResizeDown(0);
			updates.TextureBindingDescriptorsUpdates.ResizeDown(0);
			updates.BufferBindingTypes.ResizeDown(0);
		});
	}

	{
		auto& bindingsUpdate = perFrameBindingsUpdateData[frame].Materials;

		GTSL::ForEachIndexed(bindingsUpdate, [&](uint32 index, BindingsUpdateData::Updates& updates)
		{
			Vector<BindingsSet::BindingUpdateInfo, BE::TAR> bindingUpdateInfos(16, updates.BufferBindingDescriptorsUpdates.GetLength(), GetTransientAllocator());
			for (uint32 i = 0; i < bindingUpdateInfos.GetLength(); ++i)
			{
				bindingUpdateInfos[i].Type = updates.BufferBindingTypes[i];
				bindingUpdateInfos[i].ArrayElement = 0;
				bindingUpdateInfos[i].Count = updates.BufferBindingDescriptorsUpdates.GetLength();
				bindingUpdateInfos[i].BindingsUpdates = updates.BufferBindingDescriptorsUpdates.GetData();
			}

			bindingsUpdateInfo.BindingUpdateInfos = bindingUpdateInfos;

			materials[index].BindingsSets[frame].Update(bindingsUpdateInfo);
			isMaterialReady[index] = true;
			
			updates.BufferBindingDescriptorsUpdates.ResizeDown(0);
			updates.TextureBindingDescriptorsUpdates.ResizeDown(0);
			updates.BufferBindingTypes.ResizeDown(0);
		});
	}
}

void MaterialSystem::updateCounter(TaskInfo taskInfo)
{
	frame = (frame + 1) % MAX_CONCURRENT_FRAMES;
}

void MaterialSystem::onMaterialLoaded(TaskInfo taskInfo, MaterialResourceManager::OnMaterialLoadInfo onMaterialLoadInfo)
{
	auto loadInfo = DYNAMIC_CAST(MaterialLoadInfo, onMaterialLoadInfo.UserData);
	
	auto* renderSystem = loadInfo->RenderSystem;
	
	MaterialInstance instance;
	
	GTSL::Array<BindingsSetLayout, 16> bindingsSetLayouts;
	bindingsSetLayouts.PushBack(GTSL::Ranger<BindingsSetLayout>(globalBindingsSetLayout)); //global bindings
	
	{
		auto& renderGroup = renderGroups.At(onMaterialLoadInfo.RenderGroup);
		bindingsSetLayouts.EmplaceBack(renderGroup.BindingsSetLayout); //render group bindings
	}
	
	if (onMaterialLoadInfo.BindingSets.GetLength() != 0) //TODO: ADD SUPPORT FOR MULTIPLE BINDING SETS
	{
		for (auto& e : perFrameBindingsUpdateData)
		{
			e.Materials.EmplaceAt(loadInfo->Component);
			auto& updateData = e.Materials[loadInfo->Component];

			updateData.BufferBindingDescriptorsUpdates.Initialize(2, GetPersistentAllocator());
			updateData.TextureBindingDescriptorsUpdates.Initialize(2, GetPersistentAllocator());
			updateData.BufferBindingTypes.Initialize(2, GetPersistentAllocator());
		}
		
		BindingsPool::CreateInfo bindingsPoolCreateInfo;
		bindingsPoolCreateInfo.RenderDevice = loadInfo->RenderSystem->GetRenderDevice();

		if constexpr (_DEBUG)
		{
			GTSL::StaticString<64> name("Bindings pool. Material: "); name += onMaterialLoadInfo.ResourceName;
			bindingsPoolCreateInfo.Name = name.begin();
		}

		GTSL::Array<BindingsPool::DescriptorPoolSize, 10> descriptorPoolSizes;

		BindingsSetLayout::CreateInfo bindingsSetLayoutCreateInfo;
		bindingsSetLayoutCreateInfo.RenderDevice = loadInfo->RenderSystem->GetRenderDevice();
		GTSL::Array<BindingsSetLayout::BindingDescriptor, 10> bindingDescriptors;
		for (auto& e : onMaterialLoadInfo.BindingSets)
		{
			auto bindingType = GAL::BindingTypeToVulkanBindingType(e.Type);
			bindingDescriptors.PushBack(BindingsSetLayout::BindingDescriptor{ bindingType, ConvertShaderStage(e.Stage), 1, 0 });
			descriptorPoolSizes.PushBack(BindingsPool::DescriptorPoolSize{ bindingType, 3 }); //TODO: ASK FOR CORRECT NUMBER OF DESCRIPTORS
		}
		bindingsSetLayoutCreateInfo.BindingsDescriptors = bindingDescriptors;

		if constexpr (_DEBUG)
		{
			GTSL::StaticString<128> name("Bindings set layout. Material: "); name += onMaterialLoadInfo.ResourceName;
			bindingsSetLayoutCreateInfo.Name = name.begin();
		}

		instance.BindingsSetLayout = BindingsSetLayout(bindingsSetLayoutCreateInfo);

		bindingsPoolCreateInfo.DescriptorPoolSizes = descriptorPoolSizes;
		bindingsPoolCreateInfo.MaxSets = MAX_CONCURRENT_FRAMES;
		instance.BindingsPool = BindingsPool(bindingsPoolCreateInfo);

		BindingsPool::AllocateBindingsSetsInfo allocateBindingsSetsInfo;
		allocateBindingsSetsInfo.RenderDevice = loadInfo->RenderSystem->GetRenderDevice();
		allocateBindingsSetsInfo.BindingsSets = GTSL::Ranger<BindingsSet>(MAX_CONCURRENT_FRAMES, instance.BindingsSets.begin());
		allocateBindingsSetsInfo.BindingsSetLayouts = GTSL::Array<BindingsSetLayout, MAX_CONCURRENT_FRAMES>{ instance.BindingsSetLayout, instance.BindingsSetLayout, instance.BindingsSetLayout };
		allocateBindingsSetsInfo.BindingsSetDynamicBindingsCounts = GTSL::Array<uint32, 2>();

		{
			GTSL::Array<GAL::VulkanCreateInfo, MAX_CONCURRENT_FRAMES> bindingsSetsCreateInfo(MAX_CONCURRENT_FRAMES);

			if constexpr (_DEBUG)
			{
				for (uint32 j = 0; j < MAX_CONCURRENT_FRAMES; ++j)
				{
					GTSL::StaticString<64> name("BindingsSet. Material: "); name += onMaterialLoadInfo.ResourceName;

					bindingsSetsCreateInfo[j].RenderDevice = renderSystem->GetRenderDevice();
					bindingsSetsCreateInfo[j].Name = name.begin();
				}
			}

			allocateBindingsSetsInfo.BindingsSetCreateInfos = bindingsSetsCreateInfo;
		}

		instance.BindingsPool.AllocateBindingsSets(allocateBindingsSetsInfo);
		instance.BindingsSets.Resize(MAX_CONCURRENT_FRAMES);

		bindingsSetLayouts.EmplaceBack(instance.BindingsSetLayout); //instance group bindings
	}

	{
		RasterizationPipeline::CreateInfo pipelineCreateInfo;
		pipelineCreateInfo.RenderDevice = loadInfo->RenderSystem->GetRenderDevice();

		if constexpr (_DEBUG)
		{
			GTSL::StaticString<64> name("Raster pipeline. Material: "); name += onMaterialLoadInfo.ResourceName;
			pipelineCreateInfo.Name = name.begin();
		}

		{

			GTSL::Array<ShaderDataType, 10> vertexDescriptor(onMaterialLoadInfo.VertexElements.GetLength());
			for (uint32 i = 0; i < onMaterialLoadInfo.VertexElements.GetLength(); ++i)
			{
				vertexDescriptor[i] = ConvertShaderDataType(onMaterialLoadInfo.VertexElements[i]);
			}

			pipelineCreateInfo.VertexDescriptor = vertexDescriptor;
		}

		pipelineCreateInfo.IsInheritable = true;

		GTSL::Array<BindingsSetLayout, 10> bindings_set_layouts;
		bindings_set_layouts.EmplaceBack(instance.BindingsSetLayout);

		{
			PipelineLayout::CreateInfo pipelineLayout;
			pipelineLayout.RenderDevice = loadInfo->RenderSystem->GetRenderDevice();

			if constexpr (_DEBUG)
			{
				GTSL::StaticString<128> name("Pipeline Layout. Material: "); name += onMaterialLoadInfo.ResourceName;
				pipelineLayout.Name = name.begin();
			}

			pipelineLayout.BindingsSetLayouts = bindingsSetLayouts;
			instance.PipelineLayout.Initialize(pipelineLayout);
		}

		pipelineCreateInfo.PipelineDescriptor.BlendEnable = onMaterialLoadInfo.BlendEnable;
		pipelineCreateInfo.PipelineDescriptor.CullMode = onMaterialLoadInfo.CullMode;
		pipelineCreateInfo.PipelineDescriptor.DepthTest = onMaterialLoadInfo.DepthTest;
		pipelineCreateInfo.PipelineDescriptor.DepthWrite = onMaterialLoadInfo.DepthWrite;
		pipelineCreateInfo.PipelineDescriptor.StencilTest = onMaterialLoadInfo.StencilTest;
		pipelineCreateInfo.PipelineDescriptor.DepthCompareOperation = GAL::CompareOperation::LESS;
		pipelineCreateInfo.PipelineDescriptor.ColorBlendOperation = onMaterialLoadInfo.ColorBlendOperation;

		pipelineCreateInfo.PipelineDescriptor.StencilOperations.Front.CompareOperation = onMaterialLoadInfo.Front.CompareOperation;
		pipelineCreateInfo.PipelineDescriptor.StencilOperations.Front.CompareMask = onMaterialLoadInfo.Front.CompareMask;
		pipelineCreateInfo.PipelineDescriptor.StencilOperations.Front.DepthFailOperation = onMaterialLoadInfo.Front.DepthFailOperation;
		pipelineCreateInfo.PipelineDescriptor.StencilOperations.Front.FailOperation = onMaterialLoadInfo.Front.FailOperation;
		pipelineCreateInfo.PipelineDescriptor.StencilOperations.Front.PassOperation = onMaterialLoadInfo.Front.PassOperation;
		pipelineCreateInfo.PipelineDescriptor.StencilOperations.Front.Reference = onMaterialLoadInfo.Front.Reference;
		pipelineCreateInfo.PipelineDescriptor.StencilOperations.Front.WriteMask = onMaterialLoadInfo.Front.WriteMask;

		pipelineCreateInfo.PipelineDescriptor.StencilOperations.Back.CompareOperation = onMaterialLoadInfo.Back.CompareOperation;
		pipelineCreateInfo.PipelineDescriptor.StencilOperations.Back.CompareMask = onMaterialLoadInfo.Back.CompareMask;
		pipelineCreateInfo.PipelineDescriptor.StencilOperations.Back.DepthFailOperation = onMaterialLoadInfo.Back.DepthFailOperation;
		pipelineCreateInfo.PipelineDescriptor.StencilOperations.Back.FailOperation = onMaterialLoadInfo.Back.FailOperation;
		pipelineCreateInfo.PipelineDescriptor.StencilOperations.Back.PassOperation = onMaterialLoadInfo.Back.PassOperation;
		pipelineCreateInfo.PipelineDescriptor.StencilOperations.Back.Reference = onMaterialLoadInfo.Back.Reference;
		pipelineCreateInfo.PipelineDescriptor.StencilOperations.Back.WriteMask = onMaterialLoadInfo.Back.WriteMask;

		pipelineCreateInfo.SurfaceExtent = { 1280, 720 };

		{
			GTSL::Array<Shader, 10> shaders; uint32 offset = 0;
			for (uint32 i = 0; i < onMaterialLoadInfo.ShaderTypes.GetLength(); ++i)
			{
				Shader::CreateInfo create_info;
				create_info.RenderDevice = loadInfo->RenderSystem->GetRenderDevice();
				create_info.ShaderData = GTSL::Ranger<const byte>(onMaterialLoadInfo.ShaderSizes[i], onMaterialLoadInfo.DataBuffer + offset);
				shaders.EmplaceBack(create_info);
				offset += onMaterialLoadInfo.ShaderSizes[i];
			}

			GTSL::Array<Pipeline::ShaderInfo, 10> shader_infos;
			for (uint32 i = 0; i < shaders.GetLength(); ++i)
			{
				shader_infos.PushBack({ ConvertShaderType(onMaterialLoadInfo.ShaderTypes[i]), &shaders[i] });
			}

			pipelineCreateInfo.Stages = shader_infos;

			auto* frameManager = taskInfo.GameInstance->GetSystem<FrameManager>("FrameManager");
			
			auto renderPassIndex = frameManager->GetRenderPassIndex(onMaterialLoadInfo.RenderPass);
			
			auto renderPass = frameManager->GetRenderPass(renderPassIndex);
			pipelineCreateInfo.SubPass = frameManager->GetSubPassIndex(renderPassIndex, onMaterialLoadInfo.SubPass);
			pipelineCreateInfo.RenderPass = &renderPass;
			pipelineCreateInfo.PipelineLayout = &instance.PipelineLayout;
			pipelineCreateInfo.PipelineCache = renderSystem->GetPipelineCache();
			instance.Pipeline = RasterizationPipeline(pipelineCreateInfo);
		}
	}
	
	loadInfo->Buffer.Free(32, GetPersistentAllocator());
	GTSL::Delete(loadInfo, GetPersistentAllocator());

	//SETUP MATERIAL UNIFORMS FROM LOADED DATA
	{
		uint32 offset = 0;
		
		for (uint32 i = 0; i < onMaterialLoadInfo.Uniforms.GetLength(); ++i)
		{
			instance.DynamicParameters.Emplace(onMaterialLoadInfo.Uniforms[i].Name, offset);
			offset += GAL::ShaderDataTypesSize(onMaterialLoadInfo.Uniforms[i].Type);
		}

		instance.DataSize = offset;
	}

	bool materialIsReady = true;
	
	for (uint32 i = 0; i < onMaterialLoadInfo.BindingSets.GetLength(); ++i)
	{
		BindingsSet::BindingsSetUpdateInfo bindingsSetUpdateInfo;
		bindingsSetUpdateInfo.RenderDevice = loadInfo->RenderSystem->GetRenderDevice();

		for (uint32 j = 0; j < onMaterialLoadInfo.BindingSets.GetLength(); ++j)
		{
			if (onMaterialLoadInfo.BindingSets[i].Type == GAL::BindingType::UNIFORM_BUFFER_DYNAMIC)
			{
				Buffer::CreateInfo bufferInfo;

				if constexpr (_DEBUG)
				{
					GTSL::StaticString<64> name("Uniform Buffer. Material: "); name += onMaterialLoadInfo.ResourceName;
					bufferInfo.Name = name.begin();
				}
				
				bufferInfo.RenderDevice = loadInfo->RenderSystem->GetRenderDevice();
				bufferInfo.Size = 1024;
				bufferInfo.BufferType = BufferType::UNIFORM;
				instance.Buffer = Buffer(bufferInfo);

				RenderSystem::BufferScratchMemoryAllocationInfo memoryAllocationInfo;
				memoryAllocationInfo.Buffer = instance.Buffer;
				memoryAllocationInfo.Allocation = &instance.Allocation;
				renderSystem->AllocateScratchBufferMemory(memoryAllocationInfo);

				instance.BindingType = BindingType::UNIFORM_BUFFER_DYNAMIC;

				for (uint32 i = 0; i < perFrameBindingsUpdateData.GetLength(); ++i)
				{
					auto& e = perFrameBindingsUpdateData[i];

					BindingsSet::BufferBindingsUpdateInfo bufferBindingsUpdateInfo;
					bufferBindingsUpdateInfo.Buffer = instance.Buffer;
					bufferBindingsUpdateInfo.Offset = GTSL::Math::PowerOf2RoundUp(instance.DataSize * i, static_cast<uint32>(minUniformBufferOffset));
					bufferBindingsUpdateInfo.Range = instance.DataSize;

					e.Materials[loadInfo->Component].BufferBindingDescriptorsUpdates.EmplaceBack(bufferBindingsUpdateInfo);
					e.Materials[loadInfo->Component].BufferBindingTypes.EmplaceBack(instance.BindingType);
				}
			}
			else
			{
				__debugbreak();
			}
		}

		materialIsReady = false;
	}

	isMaterialReady.EmplaceAt(loadInfo->Component, materialIsReady);
	materials.EmplaceAt(loadInfo->Component, instance);
}

void MaterialSystem::onTextureLoad(TaskInfo taskInfo, TextureResourceManager::OnTextureLoadInfo onTextureLoadInfo)
{
	auto* loadInfo = DYNAMIC_CAST(TextureLoadInfo, onTextureLoadInfo.UserData);

	RenderDevice::FindSupportedImageFormat findFormat;
	findFormat.TextureTiling = TextureTiling::OPTIMAL;
	findFormat.TextureUses = TextureUses::TRANSFER_DESTINATION | TextureUses::SAMPLE;
	GTSL::Array<TextureFormat, 16> candidates; candidates.EmplaceBack(ConvertFormat(onTextureLoadInfo.TextureFormat)); candidates.EmplaceBack(TextureFormat::RGBA_I8);
	findFormat.Candidates = candidates;
	auto supportedFormat = loadInfo->RenderSystem->GetRenderDevice()->FindNearestSupportedImageFormat(findFormat);

	TextureComponent textureComponent;

	{
		if (candidates[0] != supportedFormat)
		{
			Texture::ConvertImageToFormat(onTextureLoadInfo.TextureFormat, GAL::TextureFormat::RGBA_I8, onTextureLoadInfo.Extent, GTSL::AlignedPointer<byte, 16>(onTextureLoadInfo.DataBuffer.begin()), 1);
		}

		Texture::CreateInfo textureCreateInfo;
		textureCreateInfo.RenderDevice = loadInfo->RenderSystem->GetRenderDevice();

		if constexpr (_DEBUG)
		{
			GTSL::StaticString<64> name("Texture. Texture: "); name += onTextureLoadInfo.ResourceName;
			textureCreateInfo.Name = name.begin();
		}

		textureCreateInfo.Tiling = TextureTiling::OPTIMAL;
		textureCreateInfo.Uses = TextureUses::TRANSFER_DESTINATION | TextureUses::SAMPLE;
		textureCreateInfo.Dimensions = ConvertDimension(onTextureLoadInfo.Dimensions);
		textureCreateInfo.Format = static_cast<GAL::VulkanTextureFormat>(supportedFormat);
		textureCreateInfo.Extent = onTextureLoadInfo.Extent;
		textureCreateInfo.InitialLayout = TextureLayout::UNDEFINED;
		textureCreateInfo.MipLevels = 1;

		textureComponent.Texture = Texture(textureCreateInfo);
	}

	{
		RenderSystem::AllocateLocalTextureMemoryInfo allocationInfo;
		allocationInfo.Allocation = &textureComponent.Allocation;
		allocationInfo.Texture = textureComponent.Texture;

		loadInfo->RenderSystem->AllocateLocalTextureMemory(allocationInfo);
	}

	{
		TextureView::CreateInfo textureViewCreateInfo;
		textureViewCreateInfo.RenderDevice = loadInfo->RenderSystem->GetRenderDevice();

		if constexpr (_DEBUG)
		{
			GTSL::StaticString<64> name("Texture view. Texture: "); name += onTextureLoadInfo.ResourceName;
			textureViewCreateInfo.Name = name.begin();
		}

		textureViewCreateInfo.Type = GAL::VulkanTextureType::COLOR;
		textureViewCreateInfo.Dimensions = ConvertDimension(onTextureLoadInfo.Dimensions);
		textureViewCreateInfo.Format = static_cast<GAL::VulkanTextureFormat>(supportedFormat);
		textureViewCreateInfo.Texture = textureComponent.Texture;
		textureViewCreateInfo.MipLevels = 1;

		textureComponent.TextureView = TextureView(textureViewCreateInfo);
	}

	{
		RenderSystem::TextureCopyData textureCopyData;
		textureCopyData.DestinationTexture = textureComponent.Texture;
		textureCopyData.SourceBuffer = loadInfo->Buffer;
		textureCopyData.Allocation = loadInfo->RenderAllocation;
		textureCopyData.Layout = TextureLayout::TRANSFER_DST;
		textureCopyData.Extent = onTextureLoadInfo.Extent;

		loadInfo->RenderSystem->AddTextureCopy(textureCopyData);
	}

	{
		TextureSampler::CreateInfo textureSamplerCreateInfo;
		textureSamplerCreateInfo.RenderDevice = loadInfo->RenderSystem->GetRenderDevice();

		if constexpr (_DEBUG)
		{
			GTSL::StaticString<64> name("Texture sampler. Texture: "); name += onTextureLoadInfo.ResourceName;
			textureSamplerCreateInfo.Name = name.begin();
		}

		textureSamplerCreateInfo.Anisotropy = 0;

		textureComponent.TextureSampler = TextureSampler(textureSamplerCreateInfo);
	}

	textures.EmplaceAt(loadInfo->Component, textureComponent);

	BE_LOG_MESSAGE("Loaded texture ", onTextureLoadInfo.ResourceName)

	GTSL::Delete(loadInfo, GetPersistentAllocator());

	BindingsSet::TextureBindingsUpdateInfo textureBindingsUpdateInfo;

	textureBindingsUpdateInfo.TextureView = textureComponent.TextureView;
	textureBindingsUpdateInfo.Sampler = textureComponent.TextureSampler;
	textureBindingsUpdateInfo.TextureLayout = TextureLayout::SHADER_READ_ONLY;
	for (auto& e : perFrameBindingsUpdateData)
	{
		e.Global.TextureBindingDescriptorsUpdates.EmplaceBack(textureBindingsUpdateInfo);
	}
}