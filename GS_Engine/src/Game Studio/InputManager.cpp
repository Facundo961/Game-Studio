#include "InputManager.h"

#include "EventDispatcher.h"

#include "Logger.h"
#include "Application.h"

InputManager::InputManager()
{
	KeyPressedEventId = GS::Application::GetEventDispatcherInstance()->CreateEvent();
	MouseMovedEventId = GS::Application::GetEventDispatcherInstance()->CreateEvent();
}

InputManager::~InputManager()
{
}

void InputManager::KeyPressed(Key PressedKey)
{
	GS::Application::GetEventDispatcherInstance()->Notify<KeyPressedEvent>(KeyPressedEventId, KeyPressedEvent(PressedKey));

	GS_LOG_MESSAGE("Key Pressed")
}

void InputManager::MouseMoved(const Vector2 & Pos)
{
	//Update MouseOffset.
	MouseOffset = Pos - MousePos;

	//If the mouse's position dosn't equal last frame's position update don't post an event. This is to avoid unnecesary event posts.
	if (MousePos != Pos)
	{
		GS::Application::GetEventDispatcherInstance()->Notify<MouseMovedEvent>(MouseMovedEventId, MouseMovedEvent(MouseOffset));

		//GS_LOG_MESSAGE("Mouse Moved: %f, %f", Pos.X, Pos.Y)
	}

	//Set mouse position as the current position.
	MousePos = Pos;
}
