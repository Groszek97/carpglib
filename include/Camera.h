#pragma once

//-----------------------------------------------------------------------------
struct Camera
{
	Camera() : from(-2, 2, -2), to(0, 0, 0), up(0, 1, 0), fov(PI / 4), aspect(1024.f / 768), near(0.1f), far(50.f) {}

	Matrix& GetViewProjMatrix();

	Matrix mat_view_proj;
	Vec3 from, to, up;
	float fov, aspect, near, far;
};
