#pragma once

#include "SAPI/AudioCore.h"
#include "ResourceData.h"

//class AudioResource final :
//{
//public:
//	class AudioData final : public ResourceData
//	{
//		
//	public:
//		DArray<byte> Bytes;
//		AudioChannelCount AudioChannelCount;
//		AudioSampleRate AudioSampleRate;
//		AudioBitDepth AudioBitDepth;
//	};
//
//	
//	[[nodiscard]] const char* GetName() const override { return "AudioResource"; }
//	
//private:
//	AudioData data;
//	
//	bool loadResource(const LoadResourceData& loadResourceData) override;
//	void loadFallbackResource(const GTSL::String& fullPath) override;
//	
//	[[nodiscard]] const char* getResourceTypeExtension() const override { return "wav"; }
//};
