#include "EnginePch.h"
#include "EngineCore.h"
#include "Camera.h"

void Camera::Calculate()
{
	if(!changed)
		return;
	mat_view = Matrix::CreateLookAt(from, to, up);
	mat_proj = Matrix::CreatePerspectiveFieldOfView(fov, aspect, zmin, zmax);
	mat_view_proj = mat_view * mat_proj;
	changed = false;
}
