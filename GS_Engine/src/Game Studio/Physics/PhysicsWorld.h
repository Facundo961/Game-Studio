#pragma once

#include "Math/Vector3.h"

class PhysicsWorld
{
	/**
	 * \brief Specifies the gravity acceleration of this world. Is in Meters/Seconds.
	 * Usual value will be X = 0, Y = -10, Z = 0.
	 */
	Vector3 gravity{ 0, -10, 0 };

	/**
	 * \brief Specifies how much speed the air resistance removes from entities.\n
	 * Default value is 0.0001.
	 */
	float airDensity = 0.001;

	/**
	 * \brief Defines the number of substeps used for simulation. Default is 0, which mean only one iteration will run each frame.
	 */
	uint16 simSubSteps = 0;

	void doBroadPhase();
	void doNarrowPhase();
	void solveDynamicObjects(double _UpdateTime);
	
public:
	void OnUpdate();
	
	void AddPhysicsObject();

	
	void SetGravity(const Vector3& _NewGravity) { gravity = _NewGravity; }
	void SetAirDensity(const float _NewAirDensity) { airDensity = _NewAirDensity; }

	[[nodiscard]] auto& GetGravity() const { return gravity; }
	[[nodiscard]] auto& GetAirDensity() const { return airDensity; }
};
