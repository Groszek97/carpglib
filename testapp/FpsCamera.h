#pragma once

#include "Camera.h"

struct FpsCamera : public Camera
{
	FpsCamera();
	void Update(float dt);

	float yaw, pitch;
};
