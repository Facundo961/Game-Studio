#pragma once

#include "../Core.h"

#include "Clock.h"
#include "InputManager.h"
#include "Resources/ResourceManager.h"
#include "RAPI/Window.h"
#include "Game/World.h"


namespace GS
{
	class GS_API Application : public Object
	{
		static Application* ApplicationInstance;

	protected:
		Clock ClockInstance;
		InputManager InputManagerInstance;
		ResourceManager* ResourceManagerInstance = nullptr;

		World* ActiveWorld = nullptr;
		Window* ActiveWindow = nullptr;

		bool FlaggedForClose = false;
		FString CloseReason = FString("none");

		[[nodiscard]] bool ShouldClose() const;
	public:
		Application();
		virtual ~Application();

		void Run();

		[[nodiscard]] const char* GetName() const override { return "Application"; }

		virtual const char* GetApplicationName() = 0;
		[[nodiscard]] static const char* GetEngineName() { return "Game Studio"; }
		static const char* GetEngineVersion() { return "0.0.1"; }

		static Application* Get() { return ApplicationInstance; }

		//Updates the window the application gets it's context information from.
		void SetActiveWindow(Window* _NewWindow);

		//Fires a delegate to signal that the application has been requested to close.
		void PromptClose();
		//Flags the application to close on the next update.
		void Close(const char* _Reason);

		[[nodiscard]] const Clock& GetClock() const { return ClockInstance; }
		[[nodiscard]] const InputManager& GetInputManager() const { return InputManagerInstance; }
		[[nodiscard]] const ResourceManager* GetResourceManager() const { return ResourceManagerInstance; }
		[[nodiscard]] Window* GetActiveWindow() const { return ActiveWindow; }
		[[nodiscard]] World* GetActiveWorld() const { return ActiveWorld; }
	};

	Application * CreateApplication();
}
