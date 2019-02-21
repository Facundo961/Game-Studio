#pragma once

#include "Core.h"

#include "Clock.h"
#include "Window.h"
#include "Renderer.h"
#include "EventDispatcher.h"
#include "InputManager.h"
#include "ResourceManager.h"
#include "GameInstance.h"

namespace GS
{
	GS_CLASS Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

		static Application * Get() { return ApplicationInstance; }

		//TO-DO: CHECK CONST FOR POINTERS.

		Renderer * GetRendererInstance() const { return RendererInstance; }
		EventDispatcher * GetEventDispatcherInstance() const { return EventDispatcherInstance; }
		ResourceManager * GetResourceManagerInstance() const { return ResourceManagerInstance; }
		Clock * GetClockInstance() const { return ClockInstance; }
		InputManager * GetInputManagerInstance() const { return InputManagerInstance; }
		GameInstance * GetGameInstanceInstance() const { return GameInstanceInstance; }

	private:
		Clock * ClockInstance;

		Window * WindowInstance = nullptr;

		Renderer * RendererInstance;

		EventDispatcher * EventDispatcherInstance;

		InputManager * InputManagerInstance;

		ResourceManager * ResourceManagerInstance;

		GameInstance * GameInstanceInstance;

		static Application * ApplicationInstance;
		/*int ShouldClose();*/
	};

	Application * CreateApplication();
}