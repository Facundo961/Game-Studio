#pragma once

#include "Core.h"

#include "GL.h"

#include "RendererObject.h"

GS_CLASS VAO : public RendererObject
{
public:
	VAO(size_t VertexSize);
	~VAO();

	void Bind() const override;

	void CreateVertexAttribute(int NOfElementsInThisAttribute, unsigned int DataType, unsigned char Normalize, size_t AttributeSize);

private:
	unsigned char VertexAttributeIndex = 0;

	size_t VertexSize = 0;
	size_t Offset = 0;
};

