#pragma once

#include "Core.h"

#include "Clock.h"
#include "Render\Window.h"
#include "Render\Renderer.h"
#include "InputManager.h"
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
		Clock * GetClockInstance() const { return ClockInstance; }
		InputManager * GetInputManagerInstance() const { return InputManagerInstance; }
		GameInstance * GetGameInstanceInstance() const { return GameInstanceInstance; }

	private:
		Clock * ClockInstance = nullptr;
		Window * WindowInstance = nullptr;
		Renderer * RendererInstance = nullptr;
		InputManager * InputManagerInstance = nullptr;
		GameInstance * GameInstanceInstance = nullptr;

		static Application * ApplicationInstance;

		/*int ShouldClose();*/
	};

	Application * CreateApplication();
}
