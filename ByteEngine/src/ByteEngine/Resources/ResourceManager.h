#pragma once

#include "ByteEngine/Object.h"

#include <GTSL/Id.h>
#include <GTSL/DynamicType.h>
#include <GTSL/Array.hpp>

#include "ByteEngine/Game/Tasks.h"

/**
 * \brief Used to specify a type of resource loader.
 *
 * This class will be instanced sometime during the application's lifetime to allow loading of some type of resource made possible by extension of this class.
 *
 * Every extension will allow for loading of 1 type of resource.
 */
class ResourceManager : public Object
{
public:
	ResourceManager() = default;

	ResourceManager(const UTF8* name) : Object(name) {}

	struct ResourceLoadInfo
	{
		/**
		 * \brief Name of the resource to load. Must be unique and match the name used in the editor. Is case sensitive.
		 */
		GTSL::Id64 Name;
		
		/**
		 * \brief Pointer to some data to potentially be retrieved on resource load for the client to identify the resource. Can be NULL.
		 */
		SAFE_POINTER UserData;

		/**
		 * \brief Buffer to write the loaded data to.
		 */
		GTSL::Range<byte*> DataBuffer;
		
		/**
		 * \brief Instance of game instance to call to dispatch task when resource is loaded.
		 */
		class GameInstance* GameInstance = nullptr;
		
		GTSL::Array<TaskDependency, 64> ActsOn;
	};

	struct OnResourceLoad
	{
		OnResourceLoad& operator=(const ResourceLoadInfo& resourceLoadInfo)
		{
			UserData = resourceLoadInfo.UserData;
			DataBuffer = resourceLoadInfo.DataBuffer;
			ResourceName = resourceLoadInfo.Name;
		}
		
		/**
		 * \brief Pointer to the user provided data on the resource request.
		 */
		SAFE_POINTER UserData;
		
		/**
		 * \brief Buffer where the loaded data was written to.
		 */
		GTSL::Range<byte*> DataBuffer;

		GTSL::Id64 ResourceName;
	};
};