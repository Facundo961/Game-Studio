#pragma once

#include "Core.h"

#include "RendererObject.h"

#define GL_FLOAT 0x1406

GS_CLASS VAO : public RendererObject
{
public:
	VAO(size_t VertexSize);
	~VAO();

	void Bind() const override;

	void CreateVertexAttribute(uint8 NOfElementsInThisAttribute, uint32 DataType, uint8 Normalize, size_t AttributeSize);

private:
	uint8 VertexAttributeIndex = 0;

	size_t VertexSize = 0;
	size_t Offset = 0;
};

