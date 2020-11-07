#include "TextSystem.h"

#include "MaterialSystem.h"
#include "RenderSystem.h"
#include "ByteEngine/Game/GameInstance.h"

void TextSystem::Initialize(const InitializeInfo& initializeInfo)
{
	components.Initialize(initializeInfo.ScalingFactor, GetPersistentAllocator());
}

void TextSystem::Shutdown(const ShutdownInfo& shutdownInfo)
{
}

ComponentReference TextSystem::AddText(const AddTextInfo& info)
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
			scratchBufferCreateInfo.Name = name;
		}

		{
			uint32 textureSize = 0; GAL::TextureFormat textureFormat = GAL::TextureFormat::R_I8; GTSL::Extent3D textureExtent{ 0, 0, 0, };
			//info.FontResourceManager->GetFontAtlasSizeFormatExtent(fontName, &textureSize, &textureFormat, &textureExtent);

			RenderDevice::FindSupportedImageFormat findFormatInfo;
			findFormatInfo.TextureTiling = TextureTiling::OPTIMAL;
			findFormatInfo.TextureUses = TextureUses::TRANSFER_DESTINATION | TextureUses::SAMPLE;
			GTSL::Array<TextureFormat, 16> candidates; candidates.EmplaceBack(ConvertFormat(textureFormat));
			findFormatInfo.Candidates = candidates;
			const auto supportedFormat = info.RenderSystem->GetRenderDevice()->FindNearestSupportedImageFormat(findFormatInfo);

			scratchBufferCreateInfo.Size = textureExtent.Width * textureExtent.Height * FormatSize(supportedFormat);
		}

		scratchBufferCreateInfo.BufferType = BufferType::TRANSFER_SOURCE;

		Buffer scratchBuffer;

		HostRenderAllocation allocation;

		{
			RenderSystem::BufferScratchMemoryAllocationInfo scratchMemoryAllocation;
			scratchMemoryAllocation.Buffer = &scratchBuffer;
			scratchMemoryAllocation.CreateInfo = &scratchBufferCreateInfo;
			scratchMemoryAllocation.Allocation = &allocation;
			info.RenderSystem->AllocateScratchBufferMemory(scratchMemoryAllocation);
		}

		auto* loadInfo = GTSL::New<LoadInfo>(GetPersistentAllocator(), component, info.Material, scratchBuffer, info.RenderSystem, allocation);

		//fontLoadInfo.DataBuffer = GTSL::Range<byte>(allocation.Size, static_cast<byte*>(scratchBufferData));

		fontLoadInfo.UserData = DYNAMIC_TYPE(LoadInfo, loadInfo);
	}

	info.FontResourceManager->LoadImageFont(fontLoadInfo);
	
	return ComponentReference(GetSystemId(), component);
}

void TextSystem::onFontLoad(TaskInfo taskInfo, FontResourceManager::OnFontLoadInfo loadInfo)
{
	auto* info = DYNAMIC_CAST(LoadInfo, loadInfo.UserData);

	font = loadInfo.Font;
	
	GTSL::Delete(info, GetPersistentAllocator());
}
