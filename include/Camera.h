#pragma once

//-----------------------------------------------------------------------------
struct Camera
{
	Camera() : from(-2, 5, -5), to(0, 1, 0), up(0, 1, 0), aspect(4.f / 3.f), fov(PI / 4), zmin(0.1f), zmax(100.f), changed(true) {}
	const Matrix& GetViewProj()
	{
		Update();
		return mat_view_proj;
	}

private:
	void Update();

	Matrix mat_view, mat_proj, mat_view_proj;
	Vec3 from, to, up;
	float aspect, fov, zmin, zmax;
	bool changed;
};
