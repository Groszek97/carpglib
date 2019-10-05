#include <EnginePch.h>
#include <EngineCore.h>
#include "FpsCamera.h"
#include <Input.h>

FpsCamera::FpsCamera() : yaw(0), pitch(0)
{
	from = Vec3(0, 2.f, -4.f);
	to = Vec3(0, 1.5f, 0.f);
}

void FpsCamera::Update(float dt)
{
	const Int2& dif = app::input->GetMouseDif();
	yaw += float(dif.x) / 300;
	pitch += float(dif.y) / 300;

	float forward = 0, right = 0;
	if(app::input->Down(Key::W))
		forward += 10.f;
	if(app::input->Down(Key::S))
		forward -= 10.f;
	if(app::input->Down(Key::A))
		right -= 10.f;
	if(app::input->Down(Key::D))
		right += 10.f;

	Matrix mat_rot = Matrix::Rotation(pitch, yaw, 0);

	const Vec3 forward_v(0, 0, 1);
	const Vec3 right_v(1, 0, 0);

	if(forward != 0)
		from += Vec3::Transform(forward_v, mat_rot) * forward * dt;

	if(right != 0)
		from += Vec3::Transform(right_v, mat_rot) * right * dt;

	to = from + Vec3::Transform(Vec3(0, 0, 1), mat_rot);
	changed = true;
}
