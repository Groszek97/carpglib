#pragma once

//-----------------------------------------------------------------------------
#include "Camera.h"

//-----------------------------------------------------------------------------
struct FpsCamera : public Camera
{
	FpsCamera();
	void LookAt(const Vec3& pos);
	void Update(float dt);

	float yaw, pitch;
	bool rot_changed;
};
