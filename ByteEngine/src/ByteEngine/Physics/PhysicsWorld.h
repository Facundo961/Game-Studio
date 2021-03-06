#pragma once

#include <GTSL/KeepVector.h>
#include <GTSL/Math/Vector3.h>
#include <GTSL/Math/Vector4.h>



#include "HitResult.h"
#include "ByteEngine/Game/System.h"
#include "ByteEngine/Handle.hpp"
#include "ByteEngine/Game/GameInstance.h"
#include "ByteEngine/Resources/StaticMeshResourceManager.h"

class StaticMeshResourceManager;
MAKE_HANDLE(uint32, PhysicsObject);

class PhysicsWorld : public System
{
public:
	void Initialize(const InitializeInfo& initializeInfo) override
	{
		physicsObjects.Initialize(32, GetPersistentAllocator()); updatedObjects.Initialize(32, GetPersistentAllocator());
		initializeInfo.GameInstance->AddTask("onUpdate", Task<>::Create<PhysicsWorld, &PhysicsWorld::onUpdate>(this), {}, "FrameUpdate", "RenderStart");
	}
	
	void Shutdown(const ShutdownInfo& shutdownInfo) override;

	PhysicsObjectHandle AddPhysicsObject(GameInstance* gameInstance, Id meshName, StaticMeshResourceManager* staticMeshResourceManager);

	void SetGravity(const GTSL::Vector3 newGravity) { gravity = newGravity; }
	void SetDampFactor(const float32 newDampFactor) { dampFactor = newDampFactor; }

	[[nodiscard]] auto GetGravity() const { return gravity; }
	[[nodiscard]] auto GetAirDensity() const { return dampFactor; }

	HitResult TraceRay(const GTSL::Vector3 start, const GTSL::Vector3 end);

private:
	/**
	 * \brief Specifies the gravity acceleration of this world. Is in Meters/Seconds.
	 * Usual value will be X = 0, Y = -10, Z = 0.
	 */
	GTSL::Vector3 gravity{ 0, -10, 0 };

	/**
	 * \brief Specifies how much speed the to remove from entities.\n
	 * Default value is 0.0001.
	 */
	float32 dampFactor = 0.001;

	/**
	 * \brief Defines the number of substeps used for simulation. Default is 0, which mean only one iteration will run each frame.
	 */
	uint16 simSubSteps = 0;

	GTSL::Vector<PhysicsObjectHandle, BE::PAR> updatedObjects;
	
	void doBroadPhase();
	void doNarrowPhase();
	void solveDynamicObjects(double _UpdateTime);

	void onUpdate(TaskInfo taskInfo);

	void onStaticMeshInfoLoaded(TaskInfo taskInfo, StaticMeshResourceManager* staticMeshResourceManager, StaticMeshResourceManager::StaticMeshInfo staticMeshInfo);
	
	struct PhysicsObject
	{
		GTSL::Vector4 Velocity, Acceleration, Position;
	};
	GTSL::KeepVector<PhysicsObject, BE::PAR> physicsObjects;
};
