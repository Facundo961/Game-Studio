#pragma once

#include "Core.h"

#include "Resource.h"

#include <string>

#include "FVector.hpp"

class ResourceManager
{
public:
	ResourceManager();
	~ResourceManager();

	template <typename T>
	static T * GetAsset(const std::string & Path)
	{
		for (uint16 i = 0; i < LoadedResources.size(); i++)
		{
			if (LoadedResources[i].Path == Path)
			{
				return LoadedResources[i];
			}
		}

		//SHOULD RETURN NULLPTR IF NOT ALREADY LOADED. THIS IS FOR TESTING PURPOUSES ONLY!

		return LoadAsset<T>(Path);
	}

private:
	FVector<Resource> LoadedResources;

	template <typename T>
	T * LoadAsset(const std::string & Path)
	{
		T * ptr = new T(Path);

		LoadedResources.push_back(ptr);

		return ptr;
	}
};

