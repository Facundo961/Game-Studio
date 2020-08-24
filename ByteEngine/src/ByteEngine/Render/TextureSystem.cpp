#include "TextureSystem.h"

#include "RenderSystem.h"
#include "RenderTypes.h"

void TextureSystem::Initialize(const InitializeInfo& initializeInfo)
{
	textures.Initialize(initializeInfo.ScalingFactor, GetPersistentAllocator());

	BE_LOG_MESSAGE("Initialized TextureSystem")
}

void TextureSystem::Shutdown(const ShutdownInfo& shutdownInfo)
{
	auto* renderSystem = shutdownInfo.GameInstance->GetSystem<RenderSystem>("RenderSystem");

	for(auto& e : textures)
	{
		e.TextureView.Destroy(renderSystem->GetRenderDevice());
		e.Texture.Destroy(renderSystem->GetRenderDevice());
		renderSystem->DeallocateLocalBufferMemory(e.Allocation);
	}
}

System::ComponentReference TextureSystem::CreateTexture(const CreateTextureInfo& info)
{	
	TextureResourceManager::TextureLoadInfo textureLoadInfo;
	textureLoadInfo.GameInstance = info.GameInstance;
	textureLoadInfo.Name = info.TextureName;

	textureLoadInfo.OnTextureLoadInfo = GTSL::Delegate<void(TaskInfo, TextureResourceManager::OnTextureLoadInfo)>::Create<TextureSystem, &TextureSystem::onTextureLoad>(this);

	const GTSL::Array<TaskDependency, 6> loadTaskDependencies{ { "TextureSystem", AccessType::READ_WRITE }, { "RenderSystem", AccessType::READ_WRITE } };
	
	textureLoadInfo.ActsOn = loadTaskDependencies;

	{
		Buffer::CreateInfo scratchBufferCreateInfo;
		scratchBufferCreateInfo.RenderDevice = info.RenderSystem->GetRenderDevice();

		if constexpr (_DEBUG)
		{
			GTSL::StaticString<64> name("Scratch Buffer. Texture: "); name += info.TextureName;
			scratchBufferCreateInfo.Name = name.begin();
		}
		
		scratchBufferCreateInfo.Size = info.TextureResourceManager->GetTextureSize(info.TextureName);
		scratchBufferCreateInfo.BufferType = BufferType::TRANSFER_SOURCE;

		auto scratchBuffer = Buffer(scratchBufferCreateInfo);

		//TODO: SET SIZE TO SUPPORTED FORMAT FINAL BUFFER SIZE
		
		void* scratchBufferData;
		RenderAllocation allocation;

		{
			RenderSystem::BufferScratchMemoryAllocationInfo scratchMemoryAllocation;
			scratchMemoryAllocation.Buffer = scratchBuffer;
			scratchMemoryAllocation.Allocation = &allocation;
			scratchMemoryAllocation.Data = &scratchBufferData;
			info.RenderSystem->AllocateScratchBufferMemory(scratchMemoryAllocation);
		}

		void* loadInfo;
		GTSL::New<LoadInfo>(&loadInfo, GetPersistentAllocator(), component, scratchBuffer, info.RenderSystem, allocation);

		textureLoadInfo.DataBuffer = GTSL::Ranger<byte>(allocation.Size, static_cast<byte*>(scratchBufferData));
		
		textureLoadInfo.UserData = DYNAMIC_TYPE(LoadInfo, loadInfo);
	}
	
	info.TextureResourceManager->LoadTexture(textureLoadInfo);
	
	return component++;
}

void TextureSystem::onTextureLoad(TaskInfo taskInfo, TextureResourceManager::OnTextureLoadInfo onTextureLoadInfo)
{
	auto* loadInfo = DYNAMIC_CAST(LoadInfo, onTextureLoadInfo.UserData);

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
		textureCreateInfo.SourceFormat = static_cast<GAL::VulkanTextureFormat>(supportedFormat);
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
		textureViewCreateInfo.SourceFormat = static_cast<GAL::VulkanTextureFormat>(supportedFormat);
		textureViewCreateInfo.Extent = onTextureLoadInfo.Extent;
		textureViewCreateInfo.Image = textureComponent.Texture;
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
	
	textures.Insert(loadInfo->Component, textureComponent);

	BE_LOG_MESSAGE("Loaded texture ", onTextureLoadInfo.ResourceName)
}
