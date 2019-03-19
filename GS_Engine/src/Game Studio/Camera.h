#pragma once

#include "Core.h"

#include "WorldPrimitive.h"

GS_CLASS Camera : public WorldPrimitive
{
public:
	Camera() = default;
	explicit Camera(const float FOV);
	~Camera() = default;

	float GetAperture() const { return Aperture; }
	float GetIrisHeightMultiplier() const { return IrisHeightMultiplier; }
	float GetFOV() const { return FOV; }
	float GetFocusDistance() const { return FocusDistance; }
	uint16 GetWhiteBalance() const { return WhiteBalance; }
	uint16 GetISO() const { return ISO; }

	void SetAperture(const float NewAperture) { Aperture = NewAperture; }
	void SetIrisHeightMultiplier(const float NewIrisHeightMultiplier) { IrisHeightMultiplier = NewIrisHeightMultiplier; }
	void SetFOV(const float NewFOV) { FOV = NewFOV; }
	void SetFocusDistance(const float NewFocusDistance) { FocusDistance = NewFocusDistance; }
	void SetFocusDistance(const Vector3 & Object);
	void SetWhiteBalance(const uint16 NewWhiteBalance) { WhiteBalance = NewWhiteBalance; }
	void SetISO(const uint16 NewISO) { ISO = NewISO; }

protected:
	float FOV = 45.0f;
	float FocusDistance = 0.0f;

	float Aperture = 2.8f;
	float IrisHeightMultiplier = 1.0f;

	uint16 WhiteBalance = 4000;
	uint16 ISO = 1800;
};