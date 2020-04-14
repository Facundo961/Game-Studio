#pragma once

#include "Clock.h"
#include "InputManager.h"
#include "Resources/ResourceManager.h"
#include "Game/World.h"

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
		static Application* ApplicationInstance;

	protected:
		Clock ClockInstance;
		InputManager InputManagerInstance;
		ResourceManager* ResourceManagerInstance = nullptr;

		World* ActiveWorld = nullptr;

		bool flaggedForClose = false;
		bool isInBackground = false;
		GTSL::String CloseReason;

		[[nodiscard]] bool ShouldClose() const;
	public:
		explicit Application(const ApplicationCreateInfo& ACI);
		virtual ~Application();

		virtual void OnUpdate() = 0;
		
		int Run(int argc, char** argv);

		[[nodiscard]] const char* GetName() const override { return "Application"; }

		virtual const char* GetApplicationName() = 0;
		[[nodiscard]] static const char* GetEngineName() { return "Byte Engine"; }
		static const char* GetEngineVersion() { return "0.0.1"; }

		static Application* Get() { return ApplicationInstance; }

		//Fires a Delegate to signal that the application has been requested to close.
		void PromptClose();
		//Flags the application to close on the next update.
		void Close(const char* _Reason);

		[[nodiscard]] const Clock& GetClock() const { return ClockInstance; }
		[[nodiscard]] const InputManager& GetInputManager() const { return InputManagerInstance; }
		[[nodiscard]] ResourceManager* GetResourceManager() { return ResourceManagerInstance; }
		[[nodiscard]] World* GetActiveWorld() const { return ActiveWorld; }
	};

	Application* CreateApplication();
}
