#pragma once
#include "Containers/FString.h"
#include "Utility/Extent.h"

#include "Utility/Delegate.h"
#include "Math/Vector2.h"

class nWindow
{
public:
	enum class MouseButton : uint8
	{
		LEFT_BUTTON, RIGHT_BUTTON, MIDDLE_BUTTON
	};

	enum class MouseButtonState : uint8
	{
		PRESSED, RELEASED
	};

	enum class KeyboardKeyState : uint8
	{
		PRESSED, RELEASED
	};

	enum class WindowStyle
	{
		TITLE_BAR = 0, 
	};
protected:
	Delegate<void(void)> onCloseDelegate;
	Delegate<void(const Extent2D&)> onResizeDelegate;
	Delegate<void(MouseButton, MouseButtonState)> onMouseButtonClick;
	Delegate<void(Vector2)> onMouseMove;
	Delegate<void(float)> onMouseWheelMove;
	Delegate<void(uint16, KeyboardKeyState)> onKeyEvent;
	Delegate<void(const Vector2&)> onWindowResize;
public:
	struct WindowCreateInfo
	{
		FString Name;
		Extent2D Extent;
		nWindow* ParentWindow = nullptr;
		class nApplication* Application = nullptr;
	};
	nWindow(const WindowCreateInfo& windowCreateInfo);

	virtual void SetTitle(const char* title) = 0;
	virtual void Notify() = 0;

	struct WindowIconInfo
	{
		byte* Data = nullptr;
		Extent2D Extent;
	};
	virtual void SetIcon(const WindowIconInfo& windowIconInfo) = 0;

	virtual void GetFramebufferSize(Extent2D& extent) = 0;
	virtual void GetExtent(Extent2D& extent) = 0;

	static void GetAspectRatio(const Extent2D& extent, float& aspectRatio) { aspectRatio = extent.Width / extent.Height; }
	
	void SetOnCloseDelegate(const decltype(onCloseDelegate)& delegate) { onCloseDelegate = delegate; }
	void SetOnMouseMoveDelegate(const decltype(onMouseMove)& delegate) { onMouseMove = delegate; }
	void SetOnMouseWheelMoveDelegate(const decltype(onMouseWheelMove)& delegate) { onMouseWheelMove = delegate; }
	void SetOnResizeDelegate(const decltype(onResizeDelegate)& delegate) { onResizeDelegate = delegate; }
	void SetOnMouseButtonClickDelegate(const decltype(onMouseButtonClick)& delegate) { onMouseButtonClick = delegate; }
	void SetOnWindowResizeDelegate(const decltype(onWindowResize)& delegate) { onWindowResize = delegate; }
	
	enum class WindowState
	{
		MINIMIZED, MAXIMIZED, FULLSCREEN
	};
	virtual void SetState(WindowState windowState) = 0;
};
