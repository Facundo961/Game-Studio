#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <GAL/RenderCore.h>
#include <GTSL/Buffer.h>
#include <GTSL/Delegate.hpp>
#include <GTSL/Extent.h>
#include <GTSL/FlatHashMap.h>
#include <GTSL/Vector.hpp>
#include <GTSL/Math/Vector2.h>


#include "ResourceManager.h"
#include "ByteEngine/Core.h"
#include "ByteEngine/Application/AllocatorReferences.h"

namespace GTSL {
	class Buffer;
}

class FontResourceManager : public ResourceManager
{
public:
	FontResourceManager() : ResourceManager("FontResourceManager"), fonts(4, GetPersistentAllocator()) {}
	
	struct Curve
	{
		GTSL::Vector2 p0;
		GTSL::Vector2 p1;//Bezier control point or random off glyph point
		GTSL::Vector2 p2;
		uint32 IsCurve = false;

	private:
		uint32 d;
	};

	struct FontMetaData
	{
		uint16 UnitsPerEm;
		int16 Ascender;
		int16 Descender;
		int16 LineGap;
	};

	struct Path
	{
		GTSL::Vector<Curve, BE::PersistentAllocatorReference> Curves;
	};

	struct Glyph
	{
		uint32 Character;
		int16 GlyphIndex;
		int16 NumContours;
		GTSL::Vector<Path, BE::PersistentAllocatorReference> PathList;
		uint16 AdvanceWidth;
		int16 LeftSideBearing;
		int16 BoundingBox[4];
		uint32 NumTriangles;
	};

	//MAIN STRUCT
	struct Font
	{
		uint32 FileNameHash;
		std::string FullFontName;
		std::string NameTable[25];
		std::unordered_map<uint32, int16_t> KerningTable;
		std::unordered_map<uint16, Glyph> Glyphs;
		std::map<uint32, uint16> GlyphMap;
		FontMetaData Metadata;
		uint64 LastUsed;
	};

	struct Character
	{
		GTSL::Extent2D Size;       // Size of glyph
		GTSL::Extent2D Bearing;    // Offset from baseline to left/top of glyph
		GTSL::Extent2D Position;
		uint32 Advance;    // Offset to advance to next glyph
	};
	
	struct ImageFont
	{
		std::map<char, Character> Characters;
		GTSL::Buffer ImageData;
		GTSL::Extent2D Extent;
	};
	
	Font GetFont(const GTSL::Ranger<const UTF8> fontName);

	struct OnFontLoadInfo : OnResourceLoad
	{
		ImageFont* Font;
		GAL::TextureFormat TextureFormat;
		GTSL::Extent3D Extent;
	};
	
	struct FontLoadInfo : ResourceLoadInfo
	{
		GTSL::Delegate<void(TaskInfo, OnFontLoadInfo)> OnFontLoadDelegate;
	};
	void LoadImageFont(const FontLoadInfo& fontLoadInfo);

	~FontResourceManager()
	{
		auto deallocate = [&](ImageFont& imageFont)
		{
			imageFont.ImageData.Free(8, GetPersistentAllocator());
		};
		
		GTSL::ForEach(fonts, deallocate);
	}

	void GetFontAtlasSizeFormatExtent(Id id, uint32* textureSize, GAL::TextureFormat* textureFormat, GTSL::Extent3D* extent3D);
	
private:
	int8 parseData(const char* data, Font* fontData);

	GTSL::FlatHashMap<ImageFont, BE::PersistentAllocatorReference> fonts;
};
