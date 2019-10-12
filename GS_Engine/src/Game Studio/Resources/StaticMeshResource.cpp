#include "StaticMeshResource.h"

#include "RAPI/Mesh.h"

#include "Containers/FString.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

static DArray<ShaderDataTypes> Elements = { ShaderDataTypes::FLOAT3, ShaderDataTypes::FLOAT3, ShaderDataTypes::FLOAT2, ShaderDataTypes::FLOAT3, ShaderDataTypes::FLOAT3 };
VertexDescriptor StaticMeshResource::StaticMeshVertexTypeVertexDescriptor(Elements);

StaticMeshResource::~StaticMeshResource()
{
	delete Data;
}

VertexDescriptor* StaticMeshResource::GetVertexDescriptor()
{
	return &StaticMeshVertexTypeVertexDescriptor;
}

bool StaticMeshResource::LoadResource(const FString& _Path)
{
	//Create Importer.
	Assimp::Importer Importer;

	//Create Scene and import file.
	const aiScene* Scene = Importer.ReadFile(_Path.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_ImproveCacheLocality);

	if (!Scene || Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !Scene->mRootNode)
	{
		auto Res = Importer.GetErrorString();
		return false;
	}

	//Create pointer for return.
	Data = new StaticMeshResourceData;//[Scene->mNumMeshes];	//Create new array of meshes.

	//SCAST(StaticMeshResourceData*, Data) = ProcessMesh(Scene->mMeshes[0]);

	//for (uint32 i = 0; i < Scene->mNumMeshes; i++)
	//{
	//	SCAST(StaticMeshResourceData*, Data)[i] = ProcessMesh(Scene->mMeshes[i]);
	//}

	aiMesh* InMesh = Scene->mMeshes[0];

		//We allocate a new array of vertices big enough to hold the number of vertices in this mesh and assign it to the
	//pointer found inside the mesh.
	//Data->WriteTo(0, InMesh->mNumVertices);

	SCAST(StaticMeshResourceData*, Data)->VertexArray = new Vertex[InMesh->mNumVertices];
	//Set this mesh's vertex count as the number of vertices found in this mesh.
	SCAST(StaticMeshResourceData*, Data)->VertexCount = InMesh->mNumVertices;

	//------------MESH SETUP------------

	// Loop through each vertex.
	for (uint32 i = 0; i < InMesh->mNumVertices; i++)
	{
		// Positions
		SCAST(StaticMeshResourceData*, Data)->VertexArray[i].Position.X = InMesh->mVertices[i].x;
		SCAST(StaticMeshResourceData*, Data)->VertexArray[i].Position.Y = InMesh->mVertices[i].y;
		SCAST(StaticMeshResourceData*, Data)->VertexArray[i].Position.Z = InMesh->mVertices[i].z;

		// Normals
		SCAST(StaticMeshResourceData*, Data)->VertexArray[i].Normal.X = InMesh->mNormals[i].x;
		SCAST(StaticMeshResourceData*, Data)->VertexArray[i].Normal.Y = InMesh->mNormals[i].y;
		SCAST(StaticMeshResourceData*, Data)->VertexArray[i].Normal.Z = InMesh->mNormals[i].z;

		// Texture Coordinates
		if (InMesh->mTextureCoords[0]) //We check if the pointer to texture coords is valid. (Could be NULLPTR)
		{
			//A vertex can contain up to 8 different texture coordinates.
			//Here, we are making the assumption we won't be using a model with more than one texture coordinates.
			SCAST(StaticMeshResourceData*, Data)->VertexArray[i].TextCoord.U = InMesh->mTextureCoords[0][i].x;
			SCAST(StaticMeshResourceData*, Data)->VertexArray[i].TextCoord.V = InMesh->mTextureCoords[0][i].y;
		}

		// Tangent
		//Result.VertexArray[i].Tangent.X = InMesh->mTangents[i].x;
		//Result.VertexArray[i].Tangent.Y = InMesh->mTangents[i].y;
		//Result.VertexArray[i].Tangent.Z = InMesh->mTangents[i].z;

		/*
		// BiTangent
		Result.VertexArray[i].BiTangent.X = InMesh->mBitangents[i].x;
		Result.VertexArray[i].BiTangent.Y = InMesh->mBitangents[i].y;
		Result.VertexArray[i].BiTangent.Z = InMesh->mBitangents[i].z;
		*/
	}

	//We allocate a new array of unsigned ints big enough to hold the number of indices in this mesh and assign it to the
	//pointer found inside the mesh.
	SCAST(StaticMeshResourceData*, Data)->IndexArray = new uint16[InMesh->mNumFaces * 3];

	//Wow loop through each of the mesh's faces and retrieve the corresponding vertex indices.
	for (uint32 f = 0; f < InMesh->mNumFaces; f++)
	{
		const aiFace Face = InMesh->mFaces[f];

		// Retrieve all indices of the face and store them in the indices array.
		for (uint32 i = 0; i < Face.mNumIndices; i++)
		{
			SCAST(StaticMeshResourceData*, Data)->IndexArray[SCAST(StaticMeshResourceData*, Data)->IndexCount + i] = Face.mIndices[i];
		}

		//Update the vertex count by summing the number of indices that each face we loop through has.
		SCAST(StaticMeshResourceData*, Data)->IndexCount += Face.mNumIndices;
	}

	return true;
}

void StaticMeshResource::LoadFallbackResource(const FString& _Path)
{
}

StaticMeshResource::StaticMeshResourceData* StaticMeshResource::ProcessNode(aiNode * Node, const aiScene * Scene)
{
	//Store inside MeshData a new Array of meshes.
	auto MeshData = new StaticMeshResourceData[Node->mNumMeshes];

	// Loop through each of the node's meshes (if any)
	for (unsigned int m = 0; m < Node->mNumMeshes; m++)
	{
		//Create a insertholder to store the this scene's mesh at [m].
		aiMesh * Mesh = Scene->mMeshes[Node->mMeshes[m]];

		//Store in Data at [m] a pointer to the array of vertices created for this mesh.
		MeshData[m] = ProcessMesh(Mesh);

	}

	return MeshData;
}

StaticMeshResource::StaticMeshResourceData StaticMeshResource::ProcessMesh(aiMesh * InMesh)
{
	//Create a mesh object to hold the mesh currently being processed.
	StaticMeshResourceData Result;

	//------------MESH SETUP------------

	//We allocate a new array of vertices big enough to hold the number of vertices in this mesh and assign it to the
	//pointer found inside the mesh.
	Result.VertexArray = new Vertex[InMesh->mNumVertices];

	//Set this mesh's vertex count as the number of vertices found in this mesh.
	Result.VertexCount = InMesh->mNumVertices;

	//------------MESH SETUP------------

	// Loop through each vertex.
	for (uint32 i = 0; i < InMesh->mNumVertices; i++)
	{
		// Positions
		Result.VertexArray[i].Position.X = InMesh->mVertices[i].x;
		Result.VertexArray[i].Position.Y = InMesh->mVertices[i].y;
		Result.VertexArray[i].Position.Z = InMesh->mVertices[i].z;

		// Normals
		Result.VertexArray[i].Normal.X = InMesh->mNormals[i].x;
		Result.VertexArray[i].Normal.Y = InMesh->mNormals[i].y;
		Result.VertexArray[i].Normal.Z = InMesh->mNormals[i].z;

		// Texture Coordinates
		if (InMesh->mTextureCoords[0]) //We check if the pointer to texture coords is valid. (Could be NULLPTR)
		{
			//A vertex can contain up to 8 different texture coordinates.
			//Here, we are making the assumption we won't be using a model with more than one texture coordinates.
			Result.VertexArray[i].TextCoord.U = InMesh->mTextureCoords[0][i].x;
			Result.VertexArray[i].TextCoord.V = InMesh->mTextureCoords[0][i].y;
		}

		// Tangent
		//Result.VertexArray[i].Tangent.X = InMesh->mTangents[i].x;
		//Result.VertexArray[i].Tangent.Y = InMesh->mTangents[i].y;
		//Result.VertexArray[i].Tangent.Z = InMesh->mTangents[i].z;
		
		/*
		// BiTangent
		Result.VertexArray[i].BiTangent.X = InMesh->mBitangents[i].x;
		Result.VertexArray[i].BiTangent.Y = InMesh->mBitangents[i].y;
		Result.VertexArray[i].BiTangent.Z = InMesh->mBitangents[i].z;
		*/
	}

	//We allocate a new array of unsigned ints big enough to hold the number of indices in this mesh and assign it to the
	//pointer found inside the mesh.
	Result.IndexArray = new uint16[InMesh->mNumFaces * 3];

	//Wow loop through each of the mesh's faces and retrieve the corresponding vertex indices.
	for (uint32 f = 0; f < InMesh->mNumFaces; f++)
	{
		const aiFace Face = InMesh->mFaces[f];

		// Retrieve all indices of the face and store them in the indices array.
		for (uint32 i = 0; i < Face.mNumIndices; i++)
		{
			Result.IndexArray[Result.IndexCount + i] = Face.mIndices[i];
		}

		//Update the vertex count by summing the number of indices that each face we loop through has.
		Result.IndexCount += Face.mNumIndices;
	}

	return Result;
}