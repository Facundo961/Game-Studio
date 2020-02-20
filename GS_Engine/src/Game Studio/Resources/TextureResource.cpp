#include "TextureResource.h"

#include "stb image/stb_image.h"
#include <fstream>

using namespace RAPI;

TextureResource::TextureResourceData::~TextureResourceData()
{
	stbi_image_free(ImageData);
}

bool TextureResource::loadResource(const LoadResourceData& LRD_)
{
	auto X = 0, Y = 0, NofChannels = 0;

	//Load  the image.
	const auto imgdata = stbi_load(LRD_.FullPath.c_str(), &X, &Y, &NofChannels, 0);

	if (imgdata) //If file is valid
	{
		//Load  the image.
		data.ImageData = imgdata;

		data.TextureDimensions.Width = X;
		data.TextureDimensions.Height = Y;
		data.TextureFormat = NofChannels == 4 ? ImageFormat::RGBA_I8 : ImageFormat::RGB_I8;
		data.imageDataSize = NofChannels * X * Y;

		return true;
	}

	return false;
}

void TextureResource::loadFallbackResource(const FString& _Path)
{
	data.ImageData = new byte[256 * 256 * 3];

	data.TextureDimensions.Width = 256;
	data.TextureDimensions.Height = 256;

	data.TextureFormat = ImageFormat::RGB_I8;

	for (uint16 X = 0; X < 256; ++X)
	{
		for (uint16 Y = 0; Y < 256; ++Y)
		{
			data.ImageData[X + Y + 0] = X;
			data.ImageData[X + Y + 1] = Y;
			data.ImageData[X + Y + 2] = 125;
		}
	}
}
