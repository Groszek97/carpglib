#include "EnginePch.h"
#include "EngineCore.h"
#include "Camera.h"

//=================================================================================================
Matrix& Camera::GetViewProjMatrix()
{
	if(changed)
	{
		Matrix mat_view = Matrix::CreateLookAt(from, to, up),
			mat_proj = Matrix::CreatePerspectiveFieldOfView(fov, aspect, near, far);
		mat_view_proj = mat_view * mat_proj;
	}
	return mat_view_proj;
}
