#include "Application.h"

#include "Byte Engine/Resources/AudioResourceManager.h"

void onAssert(const char* text, int line, const char* file, const char* function)
{
	BE_BASIC_LOG_ERROR("ASSERT: %s, Line: %u, File: %s, Function: %s.", text, line, file, function);
}

namespace BE
{
	Application::Application(const ApplicationCreateInfo& ACI) : systemApplication(GTSL::Application::ApplicationCreateInfo{})
	{
		applicationInstance = this;		
	}

	Application::~Application()
	{
	}

	void Application::Init()
	{
		transientAllocator = new StackAllocator(&systemAllocatorReference);
		poolAllocator = new PowerOf2PoolAllocator(&systemAllocatorReference);

		clockInstance = new Clock();
		resourceManagerInstance = new ResourceManager();
		inputManagerInstance = new InputManager();
	}
	
	int Application::Run(int argc, char** argv)
	{		
		while (!shouldClose())
		{
			systemApplication.Update();
			
			clockInstance->OnUpdate();
			
			if(isInBackground)
			{
				OnBackgroundUpdate();
			}
			else
			{
				inputManagerInstance->Update();
				OnNormalUpdate();
			}
			
			transientAllocator->Clear();
		}

		if(closeMode != CloseMode::OK)
		{
			BE_LOG_WARNING("Shutting down application!\nReason: %s", closeReason.c_str())
		}

		return 0;
	}

	void Application::PromptClose()
	{
		//CloseDelegate.Dispatch();
	}

	void Application::Close(const CloseMode closeMode, const char* reason)
	{
		if (reason) closeReason = reason;
		flaggedForClose = true;
		this->closeMode = closeMode;
	}

	bool Application::shouldClose() const
	{
		return flaggedForClose;
	}
}
