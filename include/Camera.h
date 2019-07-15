#pragma once

//-----------------------------------------------------------------------------
struct Camera
{
	Camera() : from(-2, 5, -5), to(0, 1, 0), up(0, 1, 0), aspect(4.f/3.f), fov(PI/4), zmin(0.1f), zmax(100.f), auto_aspect(true), changed(true) {}
	void Calculate();
	const Matrix& GetViewProjMatrix()
	{
		Calculate();
		return mat_view_proj;
	}

	Matrix mat_view, mat_proj, mat_view_proj;
	Vec3 from, to, up;
	float aspect, fov, zmin, zmax;
	bool auto_aspect, changed;
};
