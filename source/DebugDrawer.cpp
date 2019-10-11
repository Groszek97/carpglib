#include "EnginePch.h"
#include "EngineCore.h"
#include "DebugDrawer.h"
#include "Render.h"
#include "Camera.h"
#include "ResourceManager.h"
#include "DirectX.h"

//=================================================================================================
DebugDrawer::DebugDrawer() : render(app::render), device(app::render->GetDevice()), effect(nullptr), vb(nullptr), batch(false)
{
	vertex_decl = render->GetVertexDeclaration(VDI_COLOR);
}

//=================================================================================================
DebugDrawer::~DebugDrawer()
{
}

//=================================================================================================
void DebugDrawer::OnInit()
{
	effect = render->CompileShader("debug.fx");

	tech_simple = effect->GetTechniqueByName("tech_simple");
	tech_color = effect->GetTechniqueByName("tech_color");
	assert(tech_simple && tech_color);

	h_mat_combined = effect->GetParameterByName(nullptr, "h_mat_combined");
	h_color = effect->GetParameterByName(nullptr, "h_color");
	assert(h_mat_combined && h_color);

	shapes[(int)Shape::Box] = app::res_mgr->Load<Mesh>("box.phy");
	shapes[(int)Shape::Capsule] = app::res_mgr->Load<Mesh>("capsule.phy");
}

//=================================================================================================
void DebugDrawer::OnReset()
{
	if(effect)
		V(effect->OnLostDevice());
	SafeRelease(vb);
}

//=================================================================================================
void DebugDrawer::OnReload()
{
	if(effect)
		V(effect->OnResetDevice());
}

//=================================================================================================
void DebugDrawer::OnRelease()
{
	SafeRelease(effect);
	SafeRelease(vb);
}

//=================================================================================================
void DebugDrawer::Begin(Camera& camera)
{
	mat_view_proj = camera.GetViewProj();
	cam_pos = camera.from;
	color = Color::White;

	render->SetAlphaBlend(true);
	render->SetAlphaTest(false);
	render->SetNoZWrite(false);
	render->SetNoCulling(true);
	//render->SetWireframe(true);
}

//=================================================================================================
void DebugDrawer::End()
{
	//render->SetWireframe(false);
}

//=================================================================================================
void DebugDrawer::BeginBatch()
{
	assert(!batch);
	batch = true;
}

//=================================================================================================
void DebugDrawer::EndBatch()
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
	}

	void* ptr;
	V(vb->Lock(0, 0, &ptr, D3DLOCK_DISCARD));
	memcpy(ptr, verts.data(), verts.size() * sizeof(VColor));
	V(vb->Unlock());

	uint passes;

	V(effect->SetTechnique(tech_color));
	V(effect->SetMatrix(h_mat_combined, reinterpret_cast<const D3DXMATRIX*>(&mat_view_proj)));
	V(effect->Begin(&passes, 0));
	V(effect->BeginPass(0));

	V(device->SetVertexDeclaration(vertex_decl));
	V(device->SetStreamSource(0, vb, 0, sizeof(VColor)));

	V(device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, verts.size() / 3));

	V(effect->EndPass());
	V(effect->End());

	verts.clear();
}

//=================================================================================================
void DebugDrawer::DrawShape(Shape shape, const Matrix& m)
{
	assert(!batch);

	Mesh& mesh = *shapes[(int)shape];
	Matrix mat_combined = m * mat_view_proj;
	uint passes;

	V(effect->SetTechnique(tech_simple));
	V(effect->SetMatrix(h_mat_combined, reinterpret_cast<const D3DXMATRIX*>(&mat_combined)));
	V(effect->SetVector(h_color, reinterpret_cast<const D3DXVECTOR4*>(&color)));
	V(effect->Begin(&passes, 0));
	V(effect->BeginPass(0));

	V(device->SetVertexDeclaration(render->GetVertexDeclaration(mesh.vertex_decl)));
	V(device->SetStreamSource(0, mesh.vb, 0, mesh.vertex_size));
	V(device->SetIndices(mesh.ib));

	for(Mesh::Submesh& sub : mesh.subs)
		V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sub.min_ind, sub.n_ind, sub.first * 3, sub.tris));

	V(effect->EndPass());
	V(effect->End());
}

//=================================================================================================
void DebugDrawer::AddQuad(const Vec3(&pts)[4])
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
void DebugDrawer::AddLine(const Vec3& from, const Vec3& to, float width)
{
	assert(batch);
	uint offset = verts.size();
	verts.resize(offset + 6);
	VColor* v = verts.data() + offset;
	AddLineInternal(v, from, to, width);
}

//=================================================================================================
// https://www.gamedev.net/forums/topic/617595-solved-thick-constant-width-lines-using-quads/
// can be used to draw lines with selected pixel width
// fix for line point behind camera
void DebugDrawer::AddLineInternal(VColor* v, const Vec3& from, const Vec3& to, float width)
{
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
