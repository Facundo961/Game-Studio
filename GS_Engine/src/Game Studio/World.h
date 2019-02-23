#pragma once

#include "Core.h"

#include "Object.h"

#include "WorldObject.h"

#include "FVector.hpp"

class StaticMesh;
class Camera;

GS_CLASS World : public Object
{
public:
	World();
	~World();

	void OnUpdate() override;

	void SpawnObject(WorldObject * NewObject, const Vector3 & Position);
	void SpawnObject(StaticMesh * NewStaticMesh, const Vector3 & Position);

	void SetActiveCamera(Camera * Camera) const;

	const FVector<WorldObject *> & GetEntityList() const { return EntityList; }

protected:
	FVector<WorldObject *> EntityList;

};