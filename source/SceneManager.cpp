#include "EnginePch.h"
#include "EngineCore.h"
#include "SceneManager.h"
#include "Scene.h"
#include "SceneNode.h"
#include "Camera.h"
#include "Render.h"
#include "SuperShader.h"
#include "Mesh.h"
#include "DebugDrawer.h"
#include "DirectX.h"

SceneManager* app::scene_mgr;

SceneManager::SceneManager() : active_scene(nullptr), camera(nullptr), shader(nullptr), debug_drawer(nullptr), use_fog(true), use_lighting(true)
{
}

SceneManager::~SceneManager()
{
	DeleteElements(scenes);
	delete camera;
}

void SceneManager::Init()
{
	shader = new SuperShader;
	debug_drawer = new DebugDrawer;

	app::render->RegisterShader(shader);
	app::render->RegisterShader(debug_drawer);
}

void SceneManager::Add(Scene* scene)
{
	assert(scene);
	assert(!IsInside(scenes, scene));
	scenes.push_back(scene);
}

void SceneManager::Draw()
{
	Color clear_color;
	nodes.clear();
	if(active_scene && camera)
	{
		clear_color = active_scene->clear_color;
		active_scene->GetVisibleNodes(nodes);
	}
	else
		clear_color = Color::Black;

	IDirect3DDevice9* device = app::render->GetDevice();
	V(device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET | D3DCLEAR_STENCIL, clear_color, 1.f, 0));

	V(device->BeginScene());

	if(!nodes.empty())
		DrawNodes();

	if(!handlers.empty())
	{
		debug_drawer->Begin(*camera);
		for(DrawHandler& handler : handlers)
			handler(debug_drawer);
		debug_drawer->End();
	}

	V(device->EndScene());
}

void SceneManager::DrawNodes()
{
	/*Scene* scene = active_scene;
	shader->Prepare();
	if(use_fog && scene)
		shader->SetFog(scene->fog_range, scene->fog_color);
	else
		shader->SetFogDisabled();

	uint current_id = 0xFFFFFFFF;
	ID3DXEffect* e = shader->GetEffect();
	D3DXHANDLE tech;
	uint passes;
	V(e->FindNextValidTechnique(nullptr, &tech));
	V(e->SetTechnique(tech));
	V(e->SetVector(shader->hTint, (D3DXVECTOR4*)&Vec4::One));

	Matrix mat_view_proj = camera->GetViewProjMatrix(),
		mat_world, mat_combined;

	shader->Begin();

	for(SceneNode* node : nodes)
	{

	}

	shader->End();*/

	Scene* scene = active_scene;
	IDirect3DDevice9* device = app::render->GetDevice();
	uint current_id = 0xFFFFFFFF;
	ID3DXEffect* e = shader->GetEffect();
	D3DXHANDLE tech;
	uint passes;
	Vec4 v;
	V(e->FindNextValidTechnique(nullptr, &tech));
	V(e->SetTechnique(tech));
	V(e->SetVector(shader->hTint, reinterpret_cast<const D3DXVECTOR4*>(&Vec4::One)));
	v = scene->ambient_color;
	V(e->SetVector(shader->hAmbientColor, reinterpret_cast<const D3DXVECTOR4*>(&v)));

	bool fog = use_fog && scene->use_fog;
	if(fog)
	{
		v = Vec4(scene->fog_range.x, scene->fog_range.y, scene->fog_range.y - scene->fog_range.x, 0);
		V(e->SetVector(shader->hFogParams, reinterpret_cast<D3DXVECTOR4*>(&v)));
		v = scene->fog_color;
		V(e->SetVector(shader->hFogColor, reinterpret_cast<D3DXVECTOR4*>(&v)));
	}

	bool lighting = use_lighting && scene->use_light;
	if(lighting)
	{
		v = Vec4(scene->light_dir, 0.f);
		V(e->SetVector(shader->hLightDir, reinterpret_cast<D3DXVECTOR4*>(&v)));
		v = scene->light_color;
		V(e->SetVector(shader->hLightColor, reinterpret_cast<D3DXVECTOR4*>(&v)));
	}

	V(e->Begin(&passes, 0));
	V(e->BeginPass(0));

	Matrix mat_view_proj = camera->GetViewProjMatrix(),
		mat_world, mat_combined;

	for(SceneNode* node : nodes)
	{
		uint id = shader->GetShaderId(node->mesh_inst != nullptr, false, fog, false, false, false, lighting);
		if(id != current_id)
		{
			V(e->EndPass());
			V(e->End());
			e = shader->GetShader(id);
			V(e->FindNextValidTechnique(nullptr, &tech));
			V(e->SetTechnique(tech));
			V(e->Begin(&passes, 0));
			V(e->BeginPass(0));
			current_id = id;
		}

		Mesh& mesh = *node->mesh;

		mat_world = node->GetWorldMatrix();
		mat_combined = mat_world * mat_view_proj;

		V(e->SetMatrix(shader->hMatCombined, (D3DXMATRIX*)&mat_combined));
		V(e->SetMatrix(shader->hMatWorld, (D3DXMATRIX*)&mat_world));
		if(node->mesh_inst != nullptr)
		{
			node->mesh_inst->SetupBones();
			V(e->SetMatrixArray(shader->hMatBones, (D3DXMATRIX*)node->mesh_inst->mat_bones.data(), node->mesh_inst->mat_bones.size()));
		}
		V(device->SetVertexDeclaration(app::render->GetVertexDeclaration(mesh.vertex_decl)));
		V(device->SetStreamSource(0, mesh.vb, 0, mesh.vertex_size));
		V(device->SetIndices(mesh.ib));

		for(int i = 0; i < mesh.head.n_subs; ++i)
		{
			const Mesh::Submesh& sub = mesh.subs[i];

			// texture
			V(e->SetTexture(shader->hTexDiffuse, sub.tex->tex));
			//if(cl_normalmap && IS_SET(current_flags, SceneNode::F_NORMAL_MAP))
			//	V(e->SetTexture(shader->hTexNormal, sub.tex_normal->tex));
			//if(cl_specularmap && IS_SET(current_flags, SceneNode::F_SPECULAR_MAP))
			//	V(e->SetTexture(shader->hTexSpecular, sub.tex_specular->tex));

			// ustawienia œwiat³a
			//V(e->SetVector(shader->hSpecularColor, (D3DXVECTOR4*)&sub.specular_color));
			//V(e->SetFloat(shader->hSpecularIntensity, sub.specular_intensity));
			//V(e->SetFloat(shader->hSpecularHardness, (float)sub.specular_hardness));

			V(e->CommitChanges());
			V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sub.min_ind, sub.n_ind, sub.first * 3, sub.tris));
		}
	}

	V(e->EndPass());
	V(e->End());
}

void SceneManager::SetActiveScene(Scene* scene)
{
	assert(!scene || IsInside(scenes, scene));
	active_scene = scene;
}
