#include "EnginePch.h"
#include "EngineCore.h"
#include "BasicShader.h"
#include "Render.h"
#include "Camera.h"
#include "DirectX.h"

//=================================================================================================
BasicShader::BasicShader() : device(app::render->GetDevice()), effect(nullptr), vb(nullptr), batch(false)
{
}

//=================================================================================================
void BasicShader::OnInit()
{
	effect = app::render->CompileShader("debug.fx");

	techSimple = effect->GetTechniqueByName("techSimple");
	techColor = effect->GetTechniqueByName("techColor");
	techArea = effect->GetTechniqueByName("techArea");
	assert(techSimple && techColor && techArea);

	hMatCombined = effect->GetParameterByName(nullptr, "matCombined");
	hColor = effect->GetParameterByName(nullptr, "color");
	hPlayerPos = effect->GetParameterByName(nullptr, "playerPos");
	hRange = effect->GetParameterByName(nullptr, "range");
	assert(hMatCombined && hColor && hPlayerPos && hRange);
}

//=================================================================================================
void BasicShader::OnReset()
{
	if(effect)
		V(effect->OnLostDevice());
	SafeRelease(vb);
}

//=================================================================================================
void BasicShader::OnReload()
{
	if(effect)
		V(effect->OnResetDevice());
}

//=================================================================================================
void BasicShader::OnRelease()
{
	SafeRelease(effect);
	SafeRelease(vb);
}

//=================================================================================================
void BasicShader::Prepare(const Camera& camera)
{
	mat_view_proj = camera.mat_view_proj;
	cam_pos = camera.from;

	app::render->SetAlphaBlend(true);
	app::render->SetAlphaTest(false);
	app::render->SetNoZWrite(false);
	app::render->SetNoCulling(true);
	V(device->SetVertexDeclaration(app::render->GetVertexDeclaration(VDI_COLOR)));
	if(vb)
		V(device->SetStreamSource(0, vb, 0, sizeof(VColor)));
}

//=================================================================================================
void BasicShader::BeginBatch()
{
	assert(!batch);
	batch = true;
}

//=================================================================================================
void BasicShader::AddQuad(const Vec3(&pts)[4], const Vec4& color)
{
	assert(batch);

	verts.push_back(VColor(pts[0], color));
	verts.push_back(VColor(pts[1], color));
	verts.push_back(VColor(pts[2], color));
	verts.push_back(VColor(pts[2], color));
	verts.push_back(VColor(pts[1], color));
	verts.push_back(VColor(pts[3], color));
}

//=================================================================================================
void BasicShader::AddLine(const Vec3& from, const Vec3& to, float width, const Vec4& color)
{
	assert(batch);

	uint offset = verts.size();
	verts.resize(offset + 6);
	VColor* v = verts.data() + offset;

	width /= 2;

	Vec3 line_dir = from - to;
	Vec3 quad_normal = cam_pos - (to + from) / 2;
	Vec3 extrude_dir = line_dir.Cross(quad_normal).Normalize();

	v[0].pos = from + extrude_dir * width;
	v[1].pos = from - extrude_dir * width;
	v[2].pos = to + extrude_dir * width;
	v[3].pos = v[1].pos;
	v[4].pos = v[2].pos;
	v[5].pos = to - extrude_dir * width;
	for(int i = 0; i < 6; ++i)
		v[i].color = color;
}

//=================================================================================================
void BasicShader::EndBatch()
{
	assert(batch);
	batch = false;

	if(verts.empty())
		return;

	if(!vb || verts.size() > vb_size)
	{
		SafeRelease(vb);
		V(device->CreateVertexBuffer(verts.size() * sizeof(VColor), D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &vb, nullptr));
		vb_size = verts.size();
		V(device->SetStreamSource(0, vb, 0, sizeof(VColor)));
	}

	void* ptr;
	V(vb->Lock(0, 0, &ptr, D3DLOCK_DISCARD));
	memcpy(ptr, verts.data(), verts.size() * sizeof(VColor));
	V(vb->Unlock());

	uint passes;

	V(effect->SetTechnique(techSimple));
	V(effect->Begin(&passes, 0));
	V(effect->BeginPass(0));

	V(effect->SetMatrix(hMatCombined, (const D3DXMATRIX*)&mat_view_proj));
	V(effect->CommitChanges());

	V(device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, verts.size() / 3));

	V(effect->EndPass());
	V(effect->End());

	verts.clear();
}
