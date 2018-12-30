#pragma once

#include "Core.h"

#include "Vertex.h"

#include "DataTypes.h"

//Bind then buffer data.

//			TERMINOLOGY
//	Count: how many something are there.
//	Size: size of the object in bytes.

GS_CLASS RendererObject
{
public:
	virtual void Bind() const;
	virtual void Enable() const;

	unsigned int GetId() const { return RendererObjectId; }

protected:
	unsigned int RendererObjectId;
};