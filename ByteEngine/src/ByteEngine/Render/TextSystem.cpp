#include "TextSystem.h"

#include "MaterialSystem.h"
#include "RenderSystem.h"
#include "ByteEngine/Game/GameInstance.h"

void TextSystem::Initialize(const InitializeInfo& initializeInfo)
{
	components.Initialize(initializeInfo.ScalingFactor, GetPersistentAllocator());
	textures.Initialize(initializeInfo.ScalingFactor, GetPersistentAllocator());

	{
		MaterialSystem::AddRenderGroupInfo addRenderGroupInfo;
		addRenderGroupInfo.Name = "TextSystem";
		addRenderGroupInfo.Bindings.EmplaceBack();
		addRenderGroupInfo.Bindings.back().EmplaceBack(BindingType::STORAGE_BUFFER_DYNAMIC);

		addRenderGroupInfo.Range.EmplaceBack();
		addRenderGroupInfo.Range.back().EmplaceBack(512*512);

		addRenderGroupInfo.Size.EmplaceBack();
		addRenderGroupInfo.Size.back().EmplaceBack(1024*1024);
		
		//addRenderGroupInfo.Data.EmplaceBack();
		//addRenderGroupInfo.Data.back().EmplaceBack(GAL::ShaderDataType::FLOAT4);
		//addRenderGroupInfo.Data.back().EmplaceBack(GAL::ShaderDataType::FLOAT4); //8 floats, 32 bytes
		
		initializeInfo.GameInstance->GetSystem<MaterialSystem>("MaterialSystem")->AddRenderGroup(initializeInfo.GameInstance, addRenderGroupInfo);
	}
}

void TextSystem::Shutdown(const ShutdownInfo& shutdownInfo)
{
}

System::ComponentReference TextSystem::AddText(const AddTextInfo& info)
{
	Text text;
	text.Position = info.Position;
	text.String = info.Text;

	auto fontName = Id("FTLTLT");
	auto component = components.EmplaceBack(text);
	
	FontResourceManager::FontLoadInfo fontLoadInfo;
	fontLoadInfo.GameInstance = info.GameInstance;
	fontLoadInfo.Name = fontName;

	fontLoadInfo.OnFontLoadDelegate = GTSL::Delegate<void(TaskInfo, FontResourceManager::OnFontLoadInfo)>::Create<TextSystem, &TextSystem::onFontLoad>(this);

	const GTSL::Array<TaskDependency, 6> loadTaskDependencies{ { "TextureSystem", AccessType::READ_WRITE }, { "RenderSystem", AccessType::READ_WRITE }, { "MaterialSystem", AccessType::READ_WRITE } };

	fontLoadInfo.ActsOn = loadTaskDependencies;

	{
		Buffer::CreateInfo scratchBufferCreateInfo;
		scratchBufferCreateInfo.RenderDevice = info.RenderSystem->GetRenderDevice();

		if constexpr (_DEBUG)
		{
			GTSL::StaticString<64> name("Scratch Buffer. Font class: "); name += info.Text.begin();
			scratchBufferCreateInfo.Name = name.begin();
		}

		{
			uint32 textureSize; GAL::TextureFormat textureFormat; GTSL::Extent3D textureExtent;
			info.FontResourceManager->GetFontAtlasSizeFormatExtent(fontName, &textureSize, &textureFormat, &textureExtent);

			RenderDevice::FindSupportedImageFormat findFormatInfo;
			findFormatInfo.TextureTiling = TextureTiling::OPTIMAL;
			findFormatInfo.TextureUses = TextureUses::TRANSFER_DESTINATION | TextureUses::SAMPLE;
			GTSL::Array<TextureFormat, 16> candidates; candidates.EmplaceBack(ConvertFormat(textureFormat));
			findFormatInfo.Candidates = candidates;
			const auto supportedFormat = info.RenderSystem->GetRenderDevice()->FindNearestSupportedImageFormat(findFormatInfo);

			scratchBufferCreateInfo.Size = textureExtent.Width * textureExtent.Height * FormatSize(supportedFormat);
		}

		scratchBufferCreateInfo.BufferType = BufferType::TRANSFER_SOURCE;

		auto scratchBuffer = Buffer(scratchBufferCreateInfo);

		void* scratchBufferData; RenderAllocation allocation;

		{
			RenderSystem::BufferScratchMemoryAllocationInfo scratchMemoryAllocation;
			scratchMemoryAllocation.Buffer = scratchBuffer;
			scratchMemoryAllocation.Allocation = &allocation;
			scratchMemoryAllocation.Data = &scratchBufferData;
			info.RenderSystem->AllocateScratchBufferMemory(scratchMemoryAllocation);
		}

		auto* loadInfo = GTSL::New<LoadInfo>(GetPersistentAllocator(), component, info.Material, scratchBuffer, info.RenderSystem, allocation);

		fontLoadInfo.DataBuffer = GTSL::Ranger<byte>(allocation.Size, static_cast<byte*>(scratchBufferData));

		fontLoadInfo.UserData = DYNAMIC_TYPE(LoadInfo, loadInfo);
	}

	info.FontResourceManager->LoadImageFont(fontLoadInfo);
	
	return component;
}

void TextSystem::onFontLoad(TaskInfo taskInfo, FontResourceManager::OnFontLoadInfo loadInfo)
{
	auto* info = DYNAMIC_CAST(LoadInfo, loadInfo.UserData);

	RenderDevice::FindSupportedImageFormat findFormat;
	findFormat.TextureTiling = TextureTiling::OPTIMAL;
	findFormat.TextureUses = TextureUses::TRANSFER_DESTINATION | TextureUses::SAMPLE;
	GTSL::Array<TextureFormat, 16> candidates; candidates.EmplaceBack(ConvertFormat(loadInfo.TextureFormat));
	findFormat.Candidates = candidates;
	auto supportedFormat = info->RenderSystem->GetRenderDevice()->FindNearestSupportedImageFormat(findFormat);

	AtlasData textureComponent;

	{
		Texture::CreateInfo textureCreateInfo;
		textureCreateInfo.RenderDevice = info->RenderSystem->GetRenderDevice();

		if constexpr (_DEBUG)
		{
			GTSL::StaticString<64> name("Texture. Font atlas: "); name += loadInfo.ResourceName;
			textureCreateInfo.Name = name.begin();
		}

		textureCreateInfo.Tiling = TextureTiling::OPTIMAL;
		textureCreateInfo.Uses = TextureUses::TRANSFER_DESTINATION | TextureUses::SAMPLE;
		textureCreateInfo.Dimensions = Dimensions::SQUARE;
		textureCreateInfo.Format = static_cast<GAL::VulkanTextureFormat>(supportedFormat);
		textureCreateInfo.Extent = loadInfo.Extent;
		textureCreateInfo.InitialLayout = TextureLayout::UNDEFINED;
		textureCreateInfo.MipLevels = 1;

		textureComponent.Texture = Texture(textureCreateInfo);
	}

	{
		RenderSystem::AllocateLocalTextureMemoryInfo allocationInfo;
		allocationInfo.Allocation = &textureComponent.Allocation;
		allocationInfo.Texture = textureComponent.Texture;

		info->RenderSystem->AllocateLocalTextureMemory(allocationInfo);
	}

	{
		TextureView::CreateInfo textureViewCreateInfo;
		textureViewCreateInfo.RenderDevice = info->RenderSystem->GetRenderDevice();

		if constexpr (_DEBUG)
		{
			GTSL::StaticString<64> name("Texture view. Font atlas: "); name += loadInfo.ResourceName;
			textureViewCreateInfo.Name = name.begin();
		}

		textureViewCreateInfo.Type = GAL::VulkanTextureType::COLOR;
		textureViewCreateInfo.Dimensions = Dimensions::SQUARE;
		textureViewCreateInfo.Format = static_cast<GAL::VulkanTextureFormat>(supportedFormat);
		textureViewCreateInfo.Texture = textureComponent.Texture;
		textureViewCreateInfo.MipLevels = 1;

		textureComponent.TextureView = TextureView(textureViewCreateInfo);
	}

	{
		RenderSystem::TextureCopyData textureCopyData;
		textureCopyData.DestinationTexture = textureComponent.Texture;
		textureCopyData.SourceBuffer = info->Buffer;
		textureCopyData.Allocation = info->Allocation;
		textureCopyData.Layout = TextureLayout::TRANSFER_DST;
		textureCopyData.Extent = loadInfo.Extent;

		info->RenderSystem->AddTextureCopy(textureCopyData);
	}

	{
		TextureSampler::CreateInfo textureSamplerCreateInfo;
		textureSamplerCreateInfo.RenderDevice = info->RenderSystem->GetRenderDevice();

		if constexpr (_DEBUG)
		{
			GTSL::StaticString<64> name("Texture sampler. Font alas: "); name += loadInfo.ResourceName;
			textureSamplerCreateInfo.Name = name.begin();
		}

		textureSamplerCreateInfo.Anisotropy = 0;

		textureComponent.TextureSampler = TextureSampler(textureSamplerCreateInfo);
	}

	textures.EmplaceAt(info->Component, textureComponent);

	BE_LOG_MESSAGE("Loaded texture ", loadInfo.ResourceName)

	taskInfo.GameInstance->GetSystem<MaterialSystem>("MaterialSystem")->AddTexture(&textureComponent.TextureView, &textureComponent.TextureSampler);
	//taskInfo.GameInstance->GetSystem<MaterialSystem>("MaterialSystem")->SetMaterialParameter(info->Material, Id("Atlas"));

	font = loadInfo.Font;
	
	GTSL::Delete(info, GetPersistentAllocator());
}
