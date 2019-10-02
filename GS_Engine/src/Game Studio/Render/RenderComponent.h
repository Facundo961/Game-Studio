#pragma once

#include "Core.h"

#include "Game/Component.h"

struct RenderableInstructions;

class GS_API RenderComponent : public Component
{
protected:
	//Defines whether this render component updates it's properties during it's lifetime or if the settings found on creation are the ones that will be used for all it's lifetime.
	//All other properties won't be updated during runtime if this flag is set to true, unless stated otherwise.
	//bool IsDynamic = false;

	//Determines whether this object will be drawn on the current update. DOES NOT DEPEND ON IsDynamic.
	bool ShouldRender = true;

public:
	//bool GetIsDynamic() const { return IsDynamic; }

	//Returns whether this render component should be rendered on the current update.
	[[nodiscard]] bool GetShouldRender() const { return ShouldRender; }

	//Returns a pointer to a static renderable instructions struct.
	//This RenderableInstructions struct should have it's set of function filled out so as to be able to specify how to render this render component type.
	[[nodiscard]] virtual RenderableInstructions GetRenderableInstructions() const = 0;

	[[nodiscard]] virtual const char* GetRenderableTypeName() const = 0;
};