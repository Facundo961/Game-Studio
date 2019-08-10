#pragma once

#include "Core.h"

#include "Containers/FVector.hpp"

#include "RenderCore.h"

GS_CLASS VertexDescriptor
{
	FVector<ShaderDataTypes> Elements;

	//Size in bytes this vertex takes up.
	uint8 Size = 0;

	INLINE static uint8 ShaderDataTypesSize(ShaderDataTypes _SDT)
	{
		switch (_SDT)
		{
			case ShaderDataTypes::FLOAT:	return 4;
			case ShaderDataTypes::FLOAT2:	return 4 * 2;
			case ShaderDataTypes::FLOAT3:	return 4 * 3;
			case ShaderDataTypes::FLOAT4:	return 4 * 4;
			case ShaderDataTypes::INT:		return 4;
			case ShaderDataTypes::INT2:		return 4 * 2;
			case ShaderDataTypes::INT3:		return 4 * 3;
			case ShaderDataTypes::INT4:		return 4 * 4;
			case ShaderDataTypes::BOOL:		return 4;
			case ShaderDataTypes::MAT3:		return 4 * 3 * 3;
			case ShaderDataTypes::MAT4:		return 4 * 4 * 4;
			default:						return 0;
		}
	}
public:
	VertexDescriptor(const FVector<ShaderDataTypes>& _Elements) : Elements(_Elements)
	{
		for (auto& Element : Elements)
		{
			Size += ShaderDataTypesSize(Element);
		}
	}


	void AddElement(const ShaderDataTypes & _Element);

	[[nodiscard]] uint8 GetOffsetToMember(uint8 _Index) const 
	{
		uint8 Offset = 0;

		for (uint8 i = 0; i < _Index; ++i)
		{
			Offset += ShaderDataTypesSize(Elements[i]);
		}

		return Offset;
	}

	[[nodiscard]] ShaderDataTypes GetAttribute(uint8 _I) const { return Elements[_I]; }

	//Returns the size in bytes this vertex takes up.
	[[nodiscard]] uint8 GetSize() const { return Size; }
	[[nodiscard]] uint8 GetAttributeCount() const { return 8; }
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
GS_STRUCT MeshCreateInfo
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

GS_CLASS Mesh
{
public:
};