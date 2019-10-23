#include "EnginePch.h"
#include "EngineCore.h"
#include "FpsCamera.h"
#include "Input.h"

const float pitch_limit = PI / 2 - 0.1f;

//=================================================================================================
FpsCamera::FpsCamera()
{
	LookAt(to);
}

//=================================================================================================
void FpsCamera::LookAt(const Vec3& pos)
{
	Vec3 v = (pos - from).Normalize();
	yaw = Clip(atan2(v.x, v.z));
	pitch = asin(-v.y);
	if(pitch > pitch_limit)
		pitch = pitch_limit;
	else if(pitch < -pitch_limit)
		pitch = -pitch_limit;
	rot_changed = true;
	Update(0.f);
}

//=================================================================================================
void FpsCamera::Update(float dt)
{
	const Int2& dif = app::input->GetMouseDif();
	if(dif != Int2::Zero)
	{
		yaw = Clip(yaw + float(dif.x) / 300);
		pitch += float(dif.y) / 300;
		if(pitch > pitch_limit)
			pitch = pitch_limit;
		else if(pitch < -pitch_limit)
			pitch = -pitch_limit;
		rot_changed = true;
	}

	float forward = 0, right = 0;
	if(app::input->Down(Key::W) || app::input->Down(Key::Up))
		forward += 10.f;
	if(app::input->Down(Key::S) || app::input->Down(Key::Down))
		forward -= 10.f;
	if(app::input->Down(Key::A) || app::input->Down(Key::Left))
		right -= 10.f;
	if(app::input->Down(Key::D) || app::input->Down(Key::Right))
		right += 10.f;

	if(rot_changed || forward != 0 || right != 0)
	{
		Matrix mat_rot = Matrix::Rotation(pitch, yaw, 0);

		const Vec3 forward_v(0, 0, 1);
		const Vec3 right_v(1, 0, 0);

		if(forward != 0)
			from += Vec3::Transform(forward_v, mat_rot) * forward * dt;

		if(right != 0)
			from += Vec3::Transform(right_v, mat_rot) * right * dt;

		to = from + Vec3::Transform(Vec3(0, 0, 1), mat_rot);
		rot_changed = false;
		changed = true;
	}
}
