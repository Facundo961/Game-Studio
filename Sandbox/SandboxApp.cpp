#include <GameStudio.h>

#include <Game Studio/Utility/FlipFlop.h>
#include "Game Studio/Game/World.h"
#include "TestObject.h"
#include <Game Studio/Resources/MaterialResource.h>
#include "Game Studio/Debug/Timer.h"
#include "Game Studio/Math/GSM.hpp"
#include <Game Studio/Resources/Stream.h>

class Framebuffer;

class Sandbox final : public GS::Application
{
public:
	Sandbox() : Application(GS::ApplicationCreateInfo{"Sandbox"})
	{
		auto MatFun = [](OutStream& _OS)
		{
			FString VS(R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform Push
{
	mat4 Mat;
} inPush;
layout(binding = 0) uniform Data
{
	layout(row_major) mat4 Pos;
} inData;

layout(location = 0)in vec3 inPos;
layout(location = 1)in vec3 inTexCoords;
layout(location = 0)out vec4 tPos;

void main()
{
	tPos = inData.Pos * vec4(inPos, 1.0);
	gl_Position = tPos;
}
)");

			_OS << VS;

			FString FS(R"(
#version 450

#extension GL_ARB_separate_shader_objects : enable

layout(location = 0)in vec4 tPos;
layout(location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(tPos.x, tPos.y, tPos.z, 1);
})");

			_OS << FS;
		};

		ResourceManagerInstance->CreateResource<MaterialResource>("M_Base", MatFun);
		
		MyWorld = new World();
		ActiveWorld = MyWorld;

 		MyObject = MyWorld->CreateWorldObject<TestObject>(Vector3(0, 0, 0));
		
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