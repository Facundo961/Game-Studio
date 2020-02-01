#pragma once

/**
 * \brief Used to specify a type of resource loader. When inherited it's functions implementation should load resources as per request
 * from the ResourceManager.
 *
 * This class will be instanced sometime during the application's lifetime to allow loading of some type of resource made possible by extension of this class.
 * 
 * Every extension will allow for loading of 1 type of resource specified with a pretty name by the GetResourceTypeName() function. Users will request loading of
 * some type of resource by asking for a resource of this "pretty" name type.
 */
class SubResourceManager
{
public:
	SubResourceManager() = default;
	~SubResourceManager() = default;
	
	/**
	 * \brief Struct specifying how a resource will be loaded.
	 */
	struct LoadResourceInfo
	{
		const char* ResourceName = nullptr;
	};

	struct OnResourceLoadInfo;
	
	/**
	 * \brief Loads a resource specified by the loadResourceInfo parameter.
	 * \param loadResourceInfo Struct holding the data for how this resource will be loaded.
	 * \param onResourceLoadInfo Struct for detailing the results of this load operation.
	 * \return Boolean to indicate whether loading of this resource was successful or not.
	 */
	virtual bool LoadResource(const LoadResourceInfo& loadResourceInfo, OnResourceLoadInfo& onResourceLoadInfo);
	/**
	 * \brief Creates a resource and fills it with fallback data. This function should be called by the superior ResourceManager if the "real" resource load operation failed.
	 * Usually the resource created by this function will contain exotic data that will draw attention to itself once utilized and will alert the developer of the failed resource load. Besides it may print and error message to the console.
	 * \param loadResourceInfo Struct holding the data for how this resource will be loaded.
	 * \param onResourceLoadInfo Struct for detailing the results of this load operation.
	 */
	virtual void LoadFallback(const LoadResourceInfo& loadResourceInfo, OnResourceLoadInfo& onResourceLoadInfo);

	/**
	 * \brief Returns a string containing the name of the type of resource the SubResourceManager child class can load.
	 * \return A string containing the type name.
	 */
	[[nodiscard]] virtual const char* GetResourceTypeName() const;
};