#include "EnginePch.h"
#include "EngineCore.h"
#include "SceneManager.h"
#include "Scene.h"
#include "SceneNode.h"
#include "Camera.h"
#include "Render.h"
#include "SuperShader.h"
#include "Mesh.h"
#include "DirectX.h"

SceneManager::SceneManager() : active_scene(nullptr), shader(nullptr)
{
}

SceneManager::~SceneManager()
{
	scenes.capacity();
	DeleteElements(scenes);
}

void SceneManager::Init(Render* render)
{
	this->render = render;

	shader = new SuperShader(render);
}

void SceneManager::Add(Scene* scene)
{
	assert(scene);
	assert(!IsInside(scenes, scene));
	scenes.push_back(scene);
}

Scene* SceneManager::CreateDefaultScene()
{
	assert(!active_scene);
	active_scene = new Scene;
	active_scene->SetCamera(new Camera);
	scenes.push_back(active_scene);
	return active_scene;
}

void SceneManager::Draw()
{
	Color clear_color;
	nodes.clear();
	if(active_scene)
	{
		clear_color = active_scene->GetClearColor();
		active_scene->GetVisibleNodes(nodes);
	}
	else
		clear_color = Color::Black;

	IDirect3DDevice9* device = render->GetDevice();
	V(device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET | D3DCLEAR_STENCIL, clear_color, 1.f, 0));
	if(nodes.empty())
		return;

	V(device->BeginScene());

	uint current_id = 0xFFFFFFFF;
	ID3DXEffect* e = shader->GetEffect();
	D3DXHANDLE tech;
	uint passes;
	V(e->FindNextValidTechnique(nullptr, &tech));
	V(e->SetTechnique(tech));
	V(e->SetVector(shader->hTint, (D3DXVECTOR4*)&Vec4::One));
	V(e->Begin(&passes, 0));
	V(e->BeginPass(0));

	Matrix mat_view_proj = active_scene->GetCamera()->GetViewProjMatrix(),
		mat_world, mat_combined;

	for(SceneNode* node : nodes)
	{
		uint id = shader->GetShaderId(node->mesh_inst != nullptr, false, false, false, false, false, false);
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
		V(device->SetVertexDeclaration(render->GetVertexDeclaration(mesh.vertex_decl)));
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

	V(device->EndScene());
}

void SceneManager::SetActiveScene(Scene* scene)
{
	assert(!scene || IsInside(scenes, scene));
	active_scene = scene;
}
