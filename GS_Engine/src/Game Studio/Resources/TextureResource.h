#pragma once

#include "Core.h"

#include "Resource.h"

#include "RGB.h"

#include "ImageSize.h"
#include "RAPI/RenderCore.h"

GS_CLASS TextureResource : public Resource
{
public:
	TextureResource(const FString& _FilePath);
	~TextureResource();

	[[nodiscard]] size_t GetDataSize() const override { return TextureFormat == Format::RGBA_I8 ? 4 : 3 * (TextureDimensions.Width * TextureDimensions.Height); }

protected:
	//Used to hold the texture's dimensions once it's been loaded.
	ImageSize TextureDimensions;

	//Used to hold the number of channels this texture has.
	Format TextureFormat;

	bool LoadResource() override;
	void LoadFallbackResource() override;
};
