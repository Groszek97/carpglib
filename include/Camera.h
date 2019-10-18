#pragma once

//-----------------------------------------------------------------------------
struct Camera
{
	Camera() : from(-2, 2, -2), to(0, 0, 0), up(0, 1, 0) {}

	Matrix mat_view_proj;
	Vec3 from, to, up;
};
