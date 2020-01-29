#pragma once

#include "Core.h"

#include "Object.h"

class WorldObject;

struct ComponentCreateInfo
{
	WorldObject* Owner = nullptr;
};

class GS_API Component : public Object
{
protected:
	WorldObject* Owner = nullptr;

public:
	void SetOwner(WorldObject* _NewOwner)
	{
		Owner = _NewOwner;
	}

	WorldObject* GetOwner() { return Owner; }
};
