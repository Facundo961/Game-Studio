#pragma once

#include "Core.h"

#include "String.h"

GS_CLASS Object
{
public:
	//Methods
	Object();
	virtual ~Object();

	virtual void OnUpdate();

protected:
	bool CanTick;
};