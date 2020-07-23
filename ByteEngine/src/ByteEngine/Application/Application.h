#pragma once

#include "ByteEngine/Core.h"

#include <GTSL/Application.h>
#include <GTSL/Allocator.h>
#include <GTSL/FlatHashMap.h>
#include <GTSL/String.hpp>

#include "Clock.h"

#include "PoolAllocator.h"
#include "StackAllocator.h"
#include "SystemAllocator.h"

#include "ByteEngine/Debug/Logger.h"
#include "ByteEngine/Resources/ResourceManager.h"

class InputManager;

#include "ByteEngine/Game/GameInstance.h"

namespace BE
{	
	/**
	 * \brief Defines all the data necessary to startup a GameStudio application instance.
	 */
	struct ApplicationCreateInfo
	{
		const char* ApplicationName = nullptr;
	};
	
	class Application : public Object
	{
	public:
		[[nodiscard]] static const char* GetEngineName() { return "Byte Engine"; }
		static const char* GetEngineVersion() { return "0.0.1"; }

		static Application* Get() { return applicationInstance; }
		
		explicit Application(const ApplicationCreateInfo& ACI);
		virtual ~Application();

		void SetSystemAllocator(SystemAllocator* newSystemAllocator) { systemAllocator = newSystemAllocator; }

		virtual void Initialize() = 0;
		virtual void Shutdown() = 0;

		enum class UpdateContext : uint8
		{
			NORMAL, BACKGROUND
		};
		
		struct OnUpdateInfo
		{
			UpdateContext UpdateContext;
		};
		virtual void OnUpdate(const OnUpdateInfo& updateInfo);
		
		int Run(int argc, char** argv);
		
		virtual const char* GetApplicationName() = 0;

		//Fires a Delegate to signal that the application has been requested to close.
		void PromptClose();

		enum class CloseMode : uint8
		{
			OK, ERROR
		};
		//Flags the application to close on the next update.
		void Close(CloseMode closeMode, const GTSL::Ranger<const UTF8>& reason);

		[[nodiscard]] GTSL::StaticString<260> GetPathToApplication() const
		{
			auto path = systemApplication.GetPathToExecutable();
			path.Drop(path.FindLast('/')); return path;
		}
		
		[[nodiscard]] const Clock* GetClock() const { return clockInstance; }
		[[nodiscard]] InputManager* GetInputManager() const { return inputManagerInstance; }
		[[nodiscard]] Logger* GetLogger() const { return logger; }
		[[nodiscard]] const GTSL::Application* GetSystemApplication() const { return &systemApplication; }
		[[nodiscard]] class GameInstance* GetGameInstance() const { return gameInstance; }

		template<typename RM>
		RM* CreateResourceManager()
		{
			auto resource_manager = GTSL::SmartPointer<ResourceManager, BE::SystemAllocatorReference>::Create<RM>(systemAllocatorReference);
			return static_cast<RM*>(resourceManagers.Emplace(GTSL::Id64(resource_manager->GetName()), MakeTransferReference(resource_manager)).operator ResourceManager*());
		}
		
		[[nodiscard]] uint64 GetApplicationTicks() const { return applicationTicks; }
		ResourceManager* GetResourceManager(const GTSL::Id64 name) { return resourceManagers.At(name); }
		
		[[nodiscard]] SystemAllocator* GetSystemAllocator() const { return systemAllocator; }
		[[nodiscard]] PoolAllocator* GetNormalAllocator() { return &poolAllocator; }
		[[nodiscard]] StackAllocator* GetTransientAllocator() { return &transientAllocator; }

#ifdef BE_DEBUG
#define BE_LOG_SUCCESS(...)		BE::Application::Get()->GetLogger()->PrintObjectLog(this, BE::Logger::VerbosityLevel::SUCCESS, __VA_ARGS__);
#define BE_LOG_MESSAGE(...)		BE::Application::Get()->GetLogger()->PrintObjectLog(this, BE::Logger::VerbosityLevel::MESSAGE, __VA_ARGS__);
#define BE_LOG_WARNING(...)		BE::Application::Get()->GetLogger()->PrintObjectLog(this, BE::Logger::VerbosityLevel::WARNING, __VA_ARGS__);
#define BE_LOG_ERROR(...)		BE::Application::Get()->GetLogger()->PrintObjectLog(this, BE::Logger::VerbosityLevel::FATAL, __VA_ARGS__);
#define BE_LOG_LEVEL( Level)		BE::Application::Get()->GetLogger()->SetMinLogLevel(Level);

#define BE_BASIC_LOG_SUCCESS(...)	BE::Application::Get()->GetLogger()->PrintBasicLog(BE::Logger::VerbosityLevel::SUCCESS, __VA_ARGS__);
#define BE_BASIC_LOG_MESSAGE(...)	BE::Application::Get()->GetLogger()->PrintBasicLog(BE::Logger::VerbosityLevel::MESSAGE, __VA_ARGS__);
#define BE_BASIC_LOG_WARNING(...)	BE::Application::Get()->GetLogger()->PrintBasicLog(BE::Logger::VerbosityLevel::WARNING, __VA_ARGS__);
#define BE_BASIC_LOG_ERROR(...)		BE::Application::Get()->GetLogger()->PrintBasicLog(BE::Logger::VerbosityLevel::FATAL, __VA_ARGS__);
#else
#define BE_LOG_SUCCESS(Text, ...)
#define BE_LOG_MESSAGE(Text, ...)
#define BE_LOG_WARNING(Text, ...)
#define BE_LOG_ERROR(Text, ...)
#define BE_LOG_LEVEL(_Level)
#define BE_BASIC_LOG_SUCCESS(Text, ...)	
#define BE_BASIC_LOG_MESSAGE(Text, ...)	
#define BE_BASIC_LOG_WARNING(Text, ...)	
#define BE_BASIC_LOG_ERROR(Text, ...)	
#endif

	protected:
		GTSL::SmartPointer<Logger, BE::SystemAllocatorReference> logger;
		GTSL::SmartPointer<GameInstance, BE::SystemAllocatorReference> gameInstance;

		GTSL::FlatHashMap<GTSL::SmartPointer<ResourceManager, BE::SystemAllocatorReference>, BE::SystemAllocatorReference> resourceManagers;
		
		SystemAllocatorReference systemAllocatorReference;
		SystemAllocator* systemAllocator{ nullptr };
		PoolAllocator poolAllocator;
		StackAllocator transientAllocator;

		GTSL::Application systemApplication;

		Clock* clockInstance{ nullptr };
		InputManager* inputManagerInstance{ nullptr };

		UpdateContext updateContext{ UpdateContext::NORMAL };

		bool flaggedForClose = false;
		CloseMode closeMode{ CloseMode::OK };
		BE_DEBUG_ONLY(GTSL::String<SystemAllocatorReference> closeReason);

		uint64 applicationTicks{ 0 };
	private:
		inline static Application* applicationInstance{ nullptr };

	};

	Application* CreateApplication(GTSL::AllocatorReference* allocatorReference);
	void DestroyApplication(Application* application, GTSL::AllocatorReference* allocatorReference);
}
