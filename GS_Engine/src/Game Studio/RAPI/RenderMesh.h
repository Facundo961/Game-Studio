#pragma once

#include "Core.h"

#include "RenderCore.h"
#include "Containers/DArray.hpp"

struct GS_API VertexElement
{
	ShaderDataTypes DataType;
	uint8 Size;
};

class GS_API VertexDescriptor
{
	DArray<VertexElement> Elements;

	//Size in bytes this vertex takes up.
	uint8 Size = 0;

public:
	explicit VertexDescriptor(const DArray<ShaderDataTypes>& _Elements) : Elements(_Elements.getLength())
	{
		Elements.resize(_Elements.getLength());

		for (uint8 i = 0; i < Elements.getCapacity(); ++i)
		{
			Elements[i].Size = ShaderDataTypesSize(_Elements[i]);
			Elements[i].DataType = _Elements[i];
			Size += Elements[i].Size;
		}
	}

	[[nodiscard]] uint8 GetOffsetToMember(uint8 _Index) const 
	{
		uint8 Offset = 0;

		for (uint8 i = 0; i < _Index; ++i)
		{
			Offset += Elements[i].Size;
		}

		return Offset;
	}

	[[nodiscard]] ShaderDataTypes GetAttribute(uint8 _I) const { return Elements[_I].DataType; }

	//Returns the size in bytes this vertex takes up.
	[[nodiscard]] uint8 GetSize() const { return Size; }
	[[nodiscard]] uint8 GetAttributeCount() const { return Elements.getCapacity(); }
};

struct Vertex;

//Describes all data necessary to create a mesh.
//    Pointer to an array holding the vertices that describe the mesh.
//        Vertex* VertexData;
//    Total number of vertices found in the VertexData array.
//        uint16 VertexCount;
//    Pointer to an array holding the indices that describe the mesh.
//        uint16* IndexData;
//    Total number of indices found in the IndexData array.
//        uint16 IndexCount;
//    A vertex descriptor that defines the layout of the vertices found in VertexData.
//        VertexDescriptor VertexLayout;
struct GS_API MeshCreateInfo
{
	//Pointer to an array holding the vertices that describe the mesh.
	void* VertexData = nullptr;
	//Total number of vertices found in the VertexData array.
	uint16 VertexCount = 0;
	//Pointer to an array holding the indices that describe the mesh.
	uint16* IndexData = nullptr;
	//Total number of indices found in the IndexData array.
	uint16 IndexCount = 0;
	//A vertex descriptor that defines the layout of the vertices found in VertexData.
	VertexDescriptor* VertexLayout = nullptr;
};

class GS_API RenderMesh
{
public:
};