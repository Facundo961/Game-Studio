#include <GameStudio.h>

#include <Game Studio/RAPI/Window.h>
#include <Game Studio/Debug/Logger.h>

#include <Game Studio/Utility/FlipFlop.h>
#include "Game Studio/Game/World.h"
#include "TestObject.h"
#include <Game Studio/Resources/MaterialResource.h>
#include "Game Studio/Debug/Timer.h"
#include "Game Studio/Math/GSM.hpp"
#include <Game Studio/Resources/TextResource.h>

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

		auto MatFun = [](Archive& _OS)
		{
			FString VS("#version 450\nlayout(push_constant) uniform Push {\nmat4 Mat;\n} inPush;\nlayout(binding = 0) uniform Data {\nmat4 Pos;\n} inData;\nlayout(location = 0)in vec3 inPos;\nlayout(location = 1)in vec3 inTexCoords;\nlayout(location = 0)out vec4 tPos;\nvoid main()\n{\ngl_Position = inData.Pos * vec4(inPos, 1.0);\n}");

			_OS << VS;

			FString FS("#version 450\nlayout(location = 0)in vec4 tPos;\nlayout(location = 0) out vec4 outColor;\nvoid main()\n{\noutColor = vec4(0.8, 0.5, 0.4, 1);//tPos;\n}");

			_OS << FS;
		};

		ResourceManagerInstance->CreateResource<MaterialResource>(FString("M_Base"), MatFun);

		Get()->SetActiveWindow(Win);
		
		MyWorld = new World();
		ActiveWorld = MyWorld;
		
		//GS_ASSERT(!MyWorld);

 		MyObject = MyWorld->CreateWorldObject<TestObject>(Vector3(0, 0, 25));

		//auto D = Functor::MakeDelegate(&Window::GetAspectRatio, Win);
	}

	void OnUpdate() override
	{
		MyWorld->OnUpdate();
	}

	~Sandbox()
	{
		delete MyWorld;
		delete GetActiveWindow();
	}

	[[nodiscard]] const char* GetName() const override { return "Sandbox"; }
	const char* GetApplicationName() override { return "Sandbox"; }

	World* MyWorld = nullptr;
	TestObject* MyObject = nullptr;
	FlipFlop Flip;
};

GS::Application	* GS::CreateApplication()
{
	return new Sandbox();
}