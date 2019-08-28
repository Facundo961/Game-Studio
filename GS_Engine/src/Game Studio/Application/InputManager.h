#pragma once

#include "Core.h"

#include "Object.h"

#include "Math\Vector2.h"
#include "InputEnums.h"
#include "Containers/Array.hpp"
#include "JoystickState.h"
#include "MouseState.h"

class Window;

GS_CLASS InputManager : public Object
{
	Window * ActiveWindow = nullptr;

	MouseState Mouse;

	Array<bool, MAX_KEYBOARD_KEYS> Keys;

	Array<JoystickState, 4> JoystickStates;

	//Mouse vars
	
	//Current mouse position.
	Vector2 MousePosition;
	
	//Offset from last frame's mouse position.
	Vector2 MouseOffset;

	float ScrollWheelMovement = 0.0f;
	float ScrollWheelDelta = 0.0f;

public:
	InputManager() = default;
	~InputManager() = default;

	void SetActiveWindow(Window* _NewWindow) { ActiveWindow = _NewWindow; }

	[[nodiscard]] MouseState GetMouseState() const { return Mouse; }
	[[nodiscard]] bool GetKeyState(KeyboardKeys _Key) const { return Keys[SCAST(uint8, _Key)]; }
	[[nodiscard]] JoystickState GetJoystickState(uint8 _Joystick) const { return JoystickStates[_Joystick]; }

	void OnUpdate() override;

};

