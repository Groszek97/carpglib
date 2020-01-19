#include "EnginePch.h"
#include "EngineCore.h"
#include "TerrainShader.h"
#include "Terrain.h"
#include "Camera.h"
#include "Texture.h"
#include "Render.h"
#include "SceneManager.h"
#include "Scene.h"
#include "DirectX.h"

//=================================================================================================
TerrainShader::TerrainShader() : device(app::render->GetDevice()), vertex_decl(nullptr), effect(nullptr)
{
	const D3DVERTEXELEMENT9 decl[] = {
		{0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,	0},
		{0, 12,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_NORMAL,	0},
		{0, 24, D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TEXCOORD,	0},
		{0, 32, D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TEXCOORD,	1},
		D3DDECL_END()
	};
	V(device->CreateVertexDeclaration(decl, &vertex_decl));
}

//=================================================================================================
TerrainShader::~TerrainShader()
{
	SafeRelease(vertex_decl);
}

//=================================================================================================
void TerrainShader::OnInit()
{
	effect = app::render->CompileShader("terrain.fx");

	tech = effect->GetTechniqueByName("terrain");
	assert(tech);

	h_mat = effect->GetParameterByName(nullptr, "matCombined");
	h_world = effect->GetParameterByName(nullptr, "matWorld");
	h_tex_blend = effect->GetParameterByName(nullptr, "texBlend");
	h_tex[0] = effect->GetParameterByName(nullptr, "tex0");
	h_tex[1] = effect->GetParameterByName(nullptr, "tex1");
	h_tex[2] = effect->GetParameterByName(nullptr, "tex2");
	h_tex[3] = effect->GetParameterByName(nullptr, "tex3");
	h_tex[4] = effect->GetParameterByName(nullptr, "tex4");
	h_color_ambient = effect->GetParameterByName(nullptr, "colorAmbient");
	h_color_diffuse = effect->GetParameterByName(nullptr, "colorDiffuse");
	h_light_dir = effect->GetParameterByName(nullptr, "lightDir");
	h_fog_color = effect->GetParameterByName(nullptr, "fogColor");
	h_fog_params = effect->GetParameterByName(nullptr, "fogParam");
	assert(h_mat && h_world && h_tex_blend && h_tex[0] && h_tex[1] && h_tex[2] && h_tex[3] && h_tex[4]
		&& h_color_ambient && h_color_diffuse && h_light_dir && h_fog_color && h_fog_params);
}

//=================================================================================================
void TerrainShader::OnReset()
{
	if(effect)
		V(effect->OnLostDevice());
}

//=================================================================================================
void TerrainShader::OnReload()
{
	if(effect)
		V(effect->OnResetDevice());
}

//=================================================================================================
void TerrainShader::OnRelease()
{
	SafeRelease(effect);
}

//=================================================================================================
void TerrainShader::Draw(Scene* scene, Camera* camera, Terrain* terrain, const vector<uint>& parts)
{
	assert(scene && camera && terrain);

	app::render->SetAlphaTest(false);
	app::render->SetAlphaBlend(false);
	app::render->SetNoCulling(false);
	app::render->SetNoZWrite(false);

	V(effect->SetTechnique(tech));
	V(effect->SetMatrix(h_world, (D3DXMATRIX*)&Matrix::IdentityMatrix));
	V(effect->SetMatrix(h_mat, (D3DXMATRIX*)&camera->mat_view_proj));
	V(effect->SetTexture(h_tex_blend, terrain->GetSplatTexture()));
	TexturePtr* tex = terrain->GetTextures();
	for(int i = 0; i < 5; ++i)
		V(effect->SetTexture(h_tex[i], tex[i]->tex));
	V(effect->SetVector(h_fog_color, (D3DXVECTOR4*)&scene->GetFogColor()));
	V(effect->SetVector(h_fog_params, (D3DXVECTOR4*)&scene->GetFogParams()));
	V(effect->SetVector(h_color_ambient, (D3DXVECTOR4*)&scene->GetAmbientColor()));
	V(effect->SetVector(h_color_diffuse, (D3DXVECTOR4*)&scene->GetLightColor()));
	V(effect->SetVector(h_light_dir, (D3DXVECTOR4*)&scene->GetLightDir()));

	VB vb;
	IB ib;
	LPD3DXMESH mesh = terrain->GetMesh();
	V(mesh->GetVertexBuffer(&vb));
	V(mesh->GetIndexBuffer(&ib));
	uint n_verts, part_tris;
	terrain->GetDrawOptions(n_verts, part_tris);

	V(device->SetVertexDeclaration(vertex_decl));
	V(device->SetStreamSource(0, vb, 0, sizeof(TerrainVertex)));
	V(device->SetIndices(ib));

	uint passes;
	V(effect->Begin(&passes, 0));
	V(effect->BeginPass(0));

	for(uint part : parts)
		V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, n_verts, part_tris * part * 3, part_tris));

	V(effect->EndPass());
	V(effect->End());

	vb->Release();
	ib->Release();
}
