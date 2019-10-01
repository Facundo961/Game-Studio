#include <GameStudio.h>

#include <string>
#include <iostream>
#include <Game Studio/RAPI/Window.h>
#include <Game Studio/Debug/Logger.h>

//#include <Game Studio/Utility/FlipFlop.h>
#include "Game Studio/Game/World.h"
#include "TestObject.h"
#include "Game Studio/RAPI/RenderDevice.h"

class Framebuffer;

class Sandbox final : public GS::Application
{
public:
	Sandbox()
	{
		WindowCreateInfo WCI;
		WCI.Extent = { 1280, 720 };
		WCI.Name = "Game Studio!";
		WCI.WindowType = WindowFit::NORMAL;
		auto Win = Window::CreateWindow(WCI);

		Get()->SetActiveWindow(Win);

		MyWorld = new World();
		ActiveWorld = MyWorld;

		GS_ASSERT(!MyWorld);
		GS_ASSERT(!MyWorld->GetName());

 		MyObject = MyWorld->CreateWorldObject<TestObject>(Vector3(0, 0, 25));

		//auto D = Functor::MakeDelegate(&Window::GetAspectRatio, Win);
	}

	void OnUpdate() final override
	{
		MyWorld->OnUpdate();
	}

	~Sandbox()
	{
		delete MyWorld;
		delete GetActiveWindow();
	}

	const char* GetName() const override { return "Sandbox"; }
	const char* GetApplicationName() override { return "Sandbox"; }

	World* MyWorld = nullptr;
	TestObject* MyObject = nullptr;
	//FlipFlop Flip;
};

GS::Application	* GS::CreateApplication()
{
	return new Sandbox();
}