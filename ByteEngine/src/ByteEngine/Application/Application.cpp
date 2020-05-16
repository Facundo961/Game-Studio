#include "Application.h"


#include <iostream>
#include <string>
#include <GTSL/StaticString.hpp>

#include "ByteEngine/Resources/AudioResourceManager.h"

void onAssert(const char* text, int line, const char* file, const char* function)
{
	//BE_BASIC_LOG_ERROR("ASSERT: %s, Line: %u, File: %s, Function: %s.", text, line, file, function);
}

namespace BE
{
	Application::Application(const ApplicationCreateInfo& ACI) : systemAllocatorReference("Application"), systemApplication(GTSL::Application::ApplicationCreateInfo{})
	{
		applicationInstance = this;
	}

	Application::~Application()
	{
	}

	void Application::Init()
	{
		transientAllocator = new StackAllocator(&systemAllocatorReference);
		poolAllocator = new PoolAllocator(&systemAllocatorReference);

		Logger::LoggerCreateInfo logger_create_info;
		GTSL::StaticString<1024> path(systemApplication.GetPathToExecutable());
		path.Drop(path.FindLast('/'));
		logger_create_info.AbsolutePathToLogDirectory = path;
		logger = new Logger(logger_create_info);
		
		clockInstance = new Clock();
		resourceManagerInstance = new ResourceManager();
		inputManagerInstance = new InputManager();
	}

	void Application::Shutdown()
	{
		if (closeMode != CloseMode::OK)
		{
			BE_LOG_WARNING("Shutting down application!\nReason: %s", closeReason.c_str())
		}
		
		delete clockInstance;
		delete resourceManagerInstance;
		delete inputManagerInstance;

		logger->Shutdown();
		delete logger;

		transientAllocator->LockedClear();
		delete transientAllocator;

		poolAllocator->Free();
		delete poolAllocator;
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

		return static_cast<int>(closeMode);
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

	bool Application::shouldClose() const { return flaggedForClose; }
}

void BE::SystemAllocatorReference::allocateFunc(const uint64 size, const uint64 alignment, void** memory, uint64* allocatedSize) const { (*allocatedSize) = size;	BE::Application::Get()->GetSystemAllocator()->Allocate(size, alignment, memory); }

void BE::SystemAllocatorReference::deallocateFunc(const uint64 size, const uint64 alignment, void* memory) const { BE::Application::Get()->GetSystemAllocator()->Deallocate(size, alignment, memory); }

void BE::TransientAllocatorReference::allocateFunc(const uint64 size, const uint64 alignment, void** memory, uint64* allocatedSize) const {	BE::Application::Get()->GetTransientAllocator()->Allocate(size, alignment, memory, allocatedSize, Name); }

void BE::TransientAllocatorReference::deallocateFunc(const uint64 size, const uint64 alignment, void* memory) const { BE::Application::Get()->GetTransientAllocator()->Deallocate(size, alignment, memory, Name); }

void BE::PersistentAllocatorReference::allocateFunc(const uint64 size, const uint64 alignment, void** memory, uint64* allocatedSize) const { Application::Get()->GetNormalAllocator()->Allocate(size, alignment, memory, allocatedSize, name); }

void BE::PersistentAllocatorReference::deallocateFunc(const uint64 size, const uint64 alignment, void* memory) const { Application::Get()->GetNormalAllocator()->Deallocate(size, alignment, memory, name); }
