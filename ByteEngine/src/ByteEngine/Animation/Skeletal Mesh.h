#pragma once

#include <GTSL/Math/Vector2.h>
#include <GTSL/Math/Vector3.h>
#include <GTSL/Id.h>
#include <map>
#include <GTSL/FlatHashMap.h>
#include <GTSL/Math/Matrix4.h>


#include "ByteEngine/Application/AllocatorReferences.h"

struct SkinnableVertex
{
	GTSL::Vector3 Position;
	GTSL::Vector3 Normal;
	uint16 BoneIDs[8];
	GTSL::Vector2 TextureCoordinates;
};

struct Joint
{
	GTSL::Id64 Name;
	uint16 Parent;
	GTSL::Matrix4 Offset;
};

class Skeleton
{
	GTSL::FlatHashMap<Id, Joint, BE::PAR> bones;

public:
};
