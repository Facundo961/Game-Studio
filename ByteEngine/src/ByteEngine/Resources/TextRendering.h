#pragma once

#include <GTSL/Vector.hpp>
#include <GTSL/Math/Math.hpp>


#include "ByteEngine/Application/AllocatorReferences.h"

#include "ByteEngine/Resources/FontResourceManager.h"
#include <GTSL/Math/Vector2.h>

#include "ByteEngine/Debug/Assert.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include "stb image/stb_image_write.h"

#undef MinMax

struct LinearBezier
{
	LinearBezier(GTSL::Vector2 a, GTSL::Vector2 b) : Points{ a, b } {}
	GTSL::Vector2 Points[2];
};

struct CubicBezier
{
	CubicBezier(GTSL::Vector2 a, GTSL::Vector2 b, GTSL::Vector2 c) : Points{ a, b, c } {}
	GTSL::Vector2 Points[3];
};

struct FaceTree : public Object
{	
	FaceTree(const BE::PersistentAllocatorReference allocator) : Faces(64, allocator)
	{
		
	}

	//Lower index bands represent lower Y locations
	//Fonts are in the range 0 <-> 1
	void MakeFromPaths(const FontResourceManager::Font& font, const BE::PAR& allocator)
	{
		const auto& glyph = font.Glyphs.at(font.GlyphMap.at('w'));

		Faces.EmplaceBack();
		auto& face = Faces.back();
		face.LinearBeziers.Initialize(16, allocator);
		face.CubicBeziers.Initialize(16, allocator);

		auto minBBox = GTSL::Vector2(glyph.BoundingBox[0], glyph.BoundingBox[1]);
		auto maxBBox = GTSL::Vector2(glyph.BoundingBox[2], glyph.BoundingBox[3]);

		//BE_LOG_MESSAGE("Min BBOx: ", minBBox.X, " ", minBBox.Y)
		//BE_LOG_MESSAGE("Max BBOx: ", maxBBox.X, " ", maxBBox.Y)
		
		for(const auto& path : glyph.Paths)
		{
			for(const auto& segment : path.Segments)
			{
				if(segment.IsBezierCurve())
				{
					GTSL::Vector2 postPoints[3];
					
					//BE_LOG_MESSAGE("Pre Curve")
					//
					//BE_LOG_MESSAGE("P0: ", segment.Points[0].X, " ", segment.Points[0].Y)
					//BE_LOG_MESSAGE("CP: ", segment.Points[1].X, " ", segment.Points[1].Y)
					//BE_LOG_MESSAGE("P1: ", segment.Points[2].X, " ", segment.Points[2].Y)

					postPoints[0] = GTSL::Math::MapToRange(segment.Points[0], minBBox, maxBBox, GTSL::Vector2(0.0f, 0.0f), GTSL::Vector2(1.0f, 1.0f));
					postPoints[1] = GTSL::Math::MapToRange(segment.Points[1], minBBox, maxBBox, GTSL::Vector2(0.0f, 0.0f), GTSL::Vector2(1.0f, 1.0f));
					postPoints[2] = GTSL::Math::MapToRange(segment.Points[2], minBBox, maxBBox, GTSL::Vector2(0.0f, 0.0f), GTSL::Vector2(1.0f, 1.0f));

					//BE_LOG_MESSAGE("Post Curve")
					//
					//BE_LOG_MESSAGE("P0: ", postPoints[0].X, " ", postPoints[0].Y)
					//BE_LOG_MESSAGE("CP: ", postPoints[1].X, " ", postPoints[1].Y)
					//BE_LOG_MESSAGE("P1: ", postPoints[2].X, " ", postPoints[2].Y)
					
					face.CubicBeziers.EmplaceBack(postPoints[0], postPoints[1], postPoints[2]);
				}
				else
				{
					GTSL::Vector2 postPoints[2];
					
					//BE_LOG_MESSAGE("Pre Line")
					//
					//BE_LOG_MESSAGE("P0: ", segment.Points[0].X, " ", segment.Points[0].Y)
					//BE_LOG_MESSAGE("P1: ", segment.Points[2].X, " ", segment.Points[2].Y)

					postPoints[0] = GTSL::Math::MapToRange(segment.Points[0], minBBox, maxBBox, GTSL::Vector2(0.0f, 0.0f), GTSL::Vector2(1.0f, 1.0f));
					postPoints[1] = GTSL::Math::MapToRange(segment.Points[2], minBBox, maxBBox, GTSL::Vector2(0.0f, 0.0f), GTSL::Vector2(1.0f, 1.0f));

					//BE_LOG_MESSAGE("Post Line")
					//
					//BE_LOG_MESSAGE("P0: ", postPoints[0].X, " ", postPoints[0].Y)
					//BE_LOG_MESSAGE("P1: ", postPoints[1].X, " ", postPoints[1].Y)
					
					face.LinearBeziers.EmplaceBack(postPoints[0], postPoints[1]);
				}

				//BE_LOG_MESSAGE("");
			}
		}

		face.Bands.Initialize(BANDS, allocator);
		for(uint16 i = 0; i < BANDS; ++i)
		{
			auto& e = face.Bands[face.Bands.EmplaceBack()];
			e.Lines.Initialize(8, allocator); e.Curves.Initialize(8, allocator);
		}

		auto GetBandsForLinear = [&](const LinearBezier& linearBezier, uint16& from, uint16& to) -> void
		{
			auto height = 1.0f / (float32)(BANDS);
			auto min = 0.0f; auto max = height;

			//for(uint16 i = 0; i < BANDS; ++i)
			//{
			//	if(linearBezier.Points[0].Y >= min && linearBezier.Points[0].Y <= max)
			//	{
			//		from = i;
			//	}
			//
			//	if(linearBezier.Points[1].Y >= min && linearBezier.Points[1].Y <= max)
			//	{
			//		to = i;
			//	}
			//	
			//	min += height;
			//	max += height;
			//}
			
			from = GTSL::Math::Clamp(uint16(linearBezier.Points[0].Y * static_cast<float32>(BANDS)), uint16(0), uint16(BANDS - 1));
			to   = GTSL::Math::Clamp(uint16(linearBezier.Points[1].Y * static_cast<float32>(BANDS)), uint16(0), uint16(BANDS - 1));
		};
		
		auto GetBandsForCubic = [&](const CubicBezier& cubicBezier, uint16& from, uint16& to) -> void
		{
			auto height = 1.0f / (float32)(BANDS);
			auto min = 0.0f; auto max = height;

			//for(uint16 i = 0; i < BANDS; ++i)
			//{
			//	if(cubicBezier.Points[0].Y >= min && cubicBezier.Points[0].Y <= max)
			//	{
			//		from = i;
			//	}
			//
			//	if(cubicBezier.Points[2].Y >= min && cubicBezier.Points[2].Y <= max)
			//	{
			//		to = i;
			//	}
			//	
			//	min += height;
			//	max += height;
			//}
			
			from = GTSL::Math::Clamp(uint16(cubicBezier.Points[0].Y * static_cast<float32>(BANDS)), uint16(0), uint16(BANDS - 1));
			to   = GTSL::Math::Clamp(uint16(cubicBezier.Points[2].Y * static_cast<float32>(BANDS)), uint16(0), uint16(BANDS - 1));
		};
		
		for(uint16 l = 0; l < face.LinearBeziers.GetLength(); ++l)
		{
			uint16 from, to;
			GetBandsForLinear(face.LinearBeziers[l], from, to); GTSL::Math::MinMax(from, to, from, to);
			for(uint16 b = from; b < to + 1; ++b) { face.Bands[b].Lines.EmplaceBack(l); }
		}
		
		for(uint16 c = 0; c < face.CubicBeziers.GetLength(); ++c)
		{
			uint16 from, to;
			GetBandsForCubic(face.CubicBeziers[c], from, to); GTSL::Math::MinMax(from, to, from, to);
			for(uint16 b = from; b < to + 1; ++b) { face.Bands[b].Curves.EmplaceBack(c); }
		}
	}

	float32 Eval(GTSL::Vector2 point, GTSL::Vector2 iResolution, uint16 ch)
	{
		constexpr auto AA_LENGTH = 0.001f;

		auto getBandIndex = [](const GTSL::Vector2 pos)
		{
			return GTSL::Math::Clamp(static_cast<uint16>(pos.Y * static_cast<float32>(BANDS)), static_cast<uint16>(0), uint16(BANDS - 1));
		};
		
		auto& face = Faces[0];

		auto& band = face.Bands[getBandIndex(point)];

		float32 result = 0.0f; float32 lowestLength = 100.0f;
		
		{				
			for(uint8 i = 0; i < band.Lines.GetLength(); ++i)
			{
				auto line = face.LinearBeziers[band.Lines[i]];

				GTSL::Vector2 min, max;
				
				GTSL::Math::MinMax(line.Points[0], line.Points[1], min, max);

				if(GTSL::Math::PointInBoxProjection(min, max, point))
				{
					float32 isOnSegment;
					auto pointLine = GTSL::Math::ClosestPointOnLineSegmentToPoint(line.Points[0], line.Points[1], point, isOnSegment);
					auto dist = GTSL::Math::LengthSquared(point, pointLine);
					
					if(dist < lowestLength)
					{
						lowestLength = dist;
						auto side = GTSL::Math::TestPointToLineSide(line.Points[0], line.Points[1], point) > 0.0f ? 1.0f : 0.0f;
						result = GTSL::Math::MapToRange(GTSL::Math::Clamp(lowestLength, 0.0f, AA_LENGTH), 0.0f, AA_LENGTH, 0.0f, 1.0f) * side;
						//result = GTSL::Math::TestPointToLineSide(line.Points[0], line.Points[1], point) >= 0.0f ? 1.0f : 0.0f;
					}
				}
			}

			{
				GTSL::Vector2 closestAB, closestBC;
				
				for(uint8 i = 0; i < band.Curves.GetLength(); ++i)
				{
					const auto& curve = face.CubicBeziers[band.Curves[i]];
				
					GTSL::Vector2 min, max;
					
					GTSL::Math::MinMax(curve.Points[0], curve.Points[2], min, max);
				
					if(GTSL::Math::PointInBoxProjection(min, max, point))
					{
						float32 dist = 100.0f;

						constexpr uint16 LOOPS = 32; float32 bounds[2] = { 0.0f, 1.0f };

						uint8 sideToAdjust = 0;

						for (uint32 l = 0; l < LOOPS; ++l)
						{
							for (uint8 i = 0, ni = 1; i < 2; ++i, --ni)
							{
								auto t = GTSL::Math::Lerp(bounds[0], bounds[1], static_cast<float32>(i) / 1.0f);
								auto ab = GTSL::Math::Lerp(curve.Points[0], curve.Points[1], t);
								auto bc = GTSL::Math::Lerp(curve.Points[1], curve.Points[2], t);
								auto pos = GTSL::Math::Lerp(ab, bc, t);
								auto newDist = GTSL::Math::LengthSquared(pos, point);

								if (newDist < dist) { sideToAdjust = ni; dist = newDist; closestAB = ab; closestBC = bc; }
							}

							bounds[sideToAdjust] = (bounds[0] + bounds[1]) / 2.0f;
						}

						if (dist < lowestLength)
						{
							lowestLength = dist;

							auto side = GTSL::Math::TestPointToLineSide(closestAB, closestBC, point) > 0.0f ? 1.0f : 0.0f;
							result = GTSL::Math::MapToRange(GTSL::Math::Clamp(lowestLength, 0.0f, AA_LENGTH), 0.0f, AA_LENGTH, 0.0f, 1.0f) * side;

							//result = GTSL::Math::TestPointToLineSide(closestAB, closestBC, point) >= 0.0f ? 1.0f : 0.0f;
						}
					}
				}
			}
		}

		return result;
	}

	static constexpr uint16 BANDS = 4;
	
	void RenderChar(GTSL::Extent2D res, uint16 ch, const BE::PAR& allocator)
	{
		GTSL::Buffer buffer; buffer.Allocate(res.Width * res.Width, 8, allocator);
		
		for(uint16 xr = 0, x = 0; xr < res.Width; ++xr, ++x)
		{
			for(uint16 yr = 0, y = res.Height - 1; yr < res.Height; ++yr, --y)
			{
				buffer.GetData()[xr + yr * res.Height] = Eval(GTSL::Vector2(x / static_cast<float32>(res.Width), y / static_cast<float32>(res.Height)), GTSL::Vector2(res.Width, res.Height), ch) * 255;
			}
		}

		stbi_write_bmp("A_CharRender.bmp", res.Width, res.Height, 1, buffer.GetData());
		
		buffer.Free(8, allocator);
	}
	
	struct Band
	{
		GTSL::Vector<uint16, BE::PAR> Lines;
		GTSL::Vector<uint16, BE::PAR> Curves;
	};
	
	struct Face
	{
		GTSL::Vector<LinearBezier, BE::PersistentAllocatorReference> LinearBeziers;
		GTSL::Vector<CubicBezier, BE::PersistentAllocatorReference> CubicBeziers;

		GTSL::Vector<Band, BE::PAR> Bands;
	};
	
	GTSL::Vector<Face, BE::PAR> Faces;
};