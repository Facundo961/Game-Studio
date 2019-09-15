#include "Application.h"

GS::Application * GS::Application::ApplicationInstance = nullptr;

namespace GS
{
	Application::Application()
	{
		ApplicationInstance = this;
	}

	Application::~Application()
	{
	}

	void Application::Run()
	{
		while (!ShouldClose())
		{
			ClockInstance.OnUpdate();
			InputManagerInstance.OnUpdate();

			ActiveWindow->Update();

			OnUpdate();
		}
	}

	void Application::SetActiveWindow(Window* _NewWindow)
	{
		ActiveWindow = _NewWindow;
		InputManagerInstance.SetActiveWindow(ActiveWindow);
	}

	void Application::PromptClose()
	{
	}

	bool Application::ShouldClose() const
	{
		return ActiveWindow->GetShouldClose() || FlaggedForClose;
	}
}