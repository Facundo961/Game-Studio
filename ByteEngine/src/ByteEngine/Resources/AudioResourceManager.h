#pragma once

#include "ByteEngine/Core.h"

#include <GTSL/File.h>
#include <GTSL/FlatHashMap.h>
#include <GTSL/Buffer.hpp>
#include <GTSL/Serialize.h>

#include "ResourceManager.h"

class AudioResourceManager final : public ResourceManager
{
public:
	struct AudioResourceInfo final
	{
		uint32 ByteOffset = 0;
		uint32 Frames;
		uint8 AudioChannelCount;
		uint8 AudioSampleRate;
		uint8 AudioBitDepth;
	};

	struct LoadAudioAssetInfo : ResourceLoadInfo
	{
	};
	void LoadAudioAsset(const LoadAudioAssetInfo& loadAudioAssetInfo);

	void ReleaseAudioAsset(Id asset)
	{
		audioBytes.Remove(asset());
	}
	
	byte* GetAssetPointer(const Id id) { return audioBytes.At(id()).GetData(); }
	uint32 GetFrameCount(Id id) const { return audioResourceInfos.At(id()).Frames; }

	AudioResourceManager();

	~AudioResourceManager();
	
private:
	GTSL::File indexFile, packageFile;
	GTSL::FlatHashMap<AudioResourceInfo, BE::PersistentAllocatorReference> audioResourceInfos;
	GTSL::FlatHashMap<GTSL::Buffer<BE::PAR>, BE::PersistentAllocatorReference> audioBytes;
};

template<class ALLOCATOR>
void Insert(const AudioResourceManager::AudioResourceInfo& audioResourceInfo, GTSL::Buffer<ALLOCATOR>& buffer)
{
	Insert(audioResourceInfo.ByteOffset, buffer);
	Insert(audioResourceInfo.Frames, buffer);
	Insert(audioResourceInfo.AudioChannelCount, buffer);
	Insert(audioResourceInfo.AudioSampleRate, buffer);
	Insert(audioResourceInfo.AudioBitDepth, buffer);
}

template<class ALLOCATOR>
void Extract(AudioResourceManager::AudioResourceInfo& audioResourceInfo, GTSL::Buffer<ALLOCATOR>& buffer)
{
	Extract(audioResourceInfo.ByteOffset, buffer);
	Extract(audioResourceInfo.Frames, buffer);
	Extract(audioResourceInfo.AudioChannelCount, buffer);
	Extract(audioResourceInfo.AudioSampleRate, buffer);
	Extract(audioResourceInfo.AudioBitDepth, buffer);
}