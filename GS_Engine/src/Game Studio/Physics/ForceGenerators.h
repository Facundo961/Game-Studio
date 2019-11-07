#pragma once

#include "ForceGenerator.h"
#include "Math/Quaternion.h"
#include "Utility/Shapes/Cone.h"

class ExplosionGenerator : public ForceGenerator
{
	float blastRadius = 0;

public:
	const char* GetForceType() override { return "Explosion"; }
};

class BuoyancyGenerator : public ForceGenerator
{
	/**
	 * \brief Fluid weight(KG) per cubic meter. E.I: water is 1000kg.
	 */
	float fluidWeight = 1000;
public:
	const char* GetForceType() override { return "Buoyancy"; }
};

class MagnetGenerator : public ForceGenerator
{
public:
	const char* GetForceType() override { return "Magnet"; }
};

class WindGenerator : public ForceGenerator
{
	Vector3 windDirection;
	
public:
	const char* GetForceType() override { return "Wind"; }
};

class DirectionalWindGenerator : public ForceGenerator
{
	Quaternion windOrientation;
	Cone windDirection;
	
public:
	const char* GetForceType() override { return "Directional Wind"; }
};