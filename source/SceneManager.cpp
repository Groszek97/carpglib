#include "EnginePch.h"
#include "EngineCore.h"
#include "SceneManager.h"
#include "Scene.h"
#include "SceneNode.h"
#include "Render.h"
#include "Camera.h"
#include "SuperShader.h"
#include "MeshInstance.h"
#include "ResourceManager.h"
#include "DirectX.h"

SceneManager* app::scene_mgr;

void SceneManager::Draw()
{
	IDirect3DDevice9* device = app::render->GetDevice();
	if(!active_scene)
	{
		V(device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET | D3DCLEAR_STENCIL, Color::Black, 1.f, 0));
		return;
	}

	Scene* scene = active_scene;

	nodes.clear();
	scene->ListVisibleNodes(*camera, nodes);
	ProcessNodes();

	V(device->BeginScene());
	V(device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET | D3DCLEAR_STENCIL, scene->clear_color, 1.f, 0));

	bool fog, point_light, dir_light;

	ID3DXEffect* effect = shader->GetEffect();

	V(effect->SetVector(shader->h_camera_pos, (D3DXVECTOR4*)&camera->from));

	// apply fog
	if(scene->use_fog && use_fog)
	{
		assert(scene->fog_range.x >= 0.f && scene->fog_range.x <= scene->fog_range.y);
		fog = true;

		Vec4 value = scene->fog_color;
		V(effect->SetVector(shader->h_fog_color, (D3DXVECTOR4*)&value));

		value = Vec4(scene->fog_range.x, scene->fog_range.y, scene->fog_range.y - scene->fog_range.x, 0.f);
		V(effect->SetVector(shader->h_fog_color, (D3DXVECTOR4*)&value));
	}
	else
		fog = false;

	// apply lighting
	if(use_lighting && (scene->use_point_light || scene->use_dir_light))
	{
		Vec4 value = scene->ambient_color;
		V(effect->SetVector(shader->h_ambient_color, (D3DXVECTOR4*)&value));

		if(scene->use_point_light)
		{
			point_light = true;
			dir_light = false;
		}
		else
		{
			point_light = false;
			dir_light = true;

			value = scene->light_color;
			V(effect->SetVector(shader->h_light_color, (D3DXVECTOR4*)&value));

			value = Vec4(scene->light_dir.Normalized(), 1.f);
			V(effect->SetVector(shader->h_light_dir, (D3DXVECTOR4*)&value));
		}
	}
	else
	{
		point_light = false;
		dir_light = false;
	}

	const Matrix& mat_view_proj = camera->GetViewProj();
	const uint flags_common = shader->GetShaderId(false, false, fog, false, false, point_light, dir_light);
	Mesh* mesh = nullptr;
	uint passes;

	for(SceneNodeGroup& group : groups)
	{
		const bool animated = IsSet(group.flags, SceneNode::ANIMATED);

		uint flags = shader->GetShaderId(animated, IsSet(group.flags, SceneNode::HAVE_BINORMALS),
			false, false, false, false, false);
		flags |= flags_common;

		effect = shader->GetShader(flags);
		D3DXHANDLE tech;
		V(effect->FindNextValidTechnique(nullptr, &tech));
		V(effect->SetTechnique(tech));

		V(effect->Begin(&passes, 0));
		V(effect->BeginPass(0));

		for(auto it = nodes.begin() + group.start, end = nodes.begin() + group.end + 1; it != end; ++it)
		{
			SceneNode* node = *it;
			const Matrix& mat_world = node->GetWorldMatrix();
			Matrix mat_combined = mat_world * mat_view_proj;

			// set mesh
			if(node->mesh != mesh)
			{
				mesh = node->mesh;
				V(device->SetVertexDeclaration(app::render->GetVertexDeclaration(mesh->vertex_decl)));
				V(device->SetStreamSource(0, mesh->vb, 0, mesh->vertex_size));
				V(device->SetIndices(mesh->ib));
			}

			V(effect->SetMatrix(shader->h_mat_combined, (D3DXMATRIX*)&mat_combined));
			V(effect->SetMatrix(shader->h_mat_world, (D3DXMATRIX*)&mat_world));
			//V(effect->SetVector(shader->hTint, (D3DXVECTOR4*)&node->tint));
			if(animated)
			{
				MeshInstance& mesh_inst = *node->mesh_inst;
				V(effect->SetMatrixArray(shader->h_mat_bones, (D3DXMATRIX*)mesh_inst.mat_bones.data(), mesh_inst.mat_bones.size()));
			}

			for(int i = 0; i < mesh->head.n_subs; ++i)
			{
				//if(!IsSet(node->subs, 1 << i))
				//	continue;

				const Mesh::Submesh& sub = mesh->subs[i];

				// set texture
				V(effect->SetTexture(shader->h_tex_diffuse, mesh->GetTexture(i)));
				//if(cl_normalmap && IsSet(current_flags, SceneNode::F_NORMAL_MAP))
				//	V(e->SetTexture(super_shader->hTexNormal, sub.tex_normal->tex));
				//if(cl_specularmap && IsSet(current_flags, SceneNode::F_SPECULAR_MAP))
				//	V(e->SetTexture(super_shader->hTexSpecular, sub.tex_specular->tex));

				// ustawienia œwiat³a
				//V(e->SetVector(super_shader->hSpecularColor, (D3DXVECTOR4*)&sub.specular_color));
				//V(e->SetFloat(super_shader->hSpecularIntensity, sub.specular_intensity));
				//V(e->SetFloat(super_shader->hSpecularHardness, (float)sub.specular_hardness));

				V(effect->CommitChanges());
				V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sub.min_ind, sub.n_ind, sub.first * 3, sub.tris));
			}
		}

		V(effect->EndPass());
		V(effect->End());
	}

	V(device->EndScene());
}

void SceneManager::ProcessNodes()
{
	if(nodes.empty())
		return;

	for(SceneNode* node : nodes)
	{
		node->tmp_flags = node->flags;
		if(!node->mesh->IsLoaded())
			app::res_mgr->LoadInstant(node->mesh);
		if(node->mesh_inst)
			node->mesh_inst->SetupBones();
	}

	std::sort(nodes.begin(), nodes.end(), [](const SceneNode* node1, const SceneNode* node2)
	{
		if(node1->tmp_flags != node2->tmp_flags)
			return node1->tmp_flags > node2->tmp_flags;
		else
			return node1->mesh > node2->mesh;
	});

	int prev_flags = -1, index = 0;
	groups.clear();
	for(SceneNode* node : nodes)
	{
		if(node->tmp_flags != prev_flags)
		{
			if(!groups.empty())
				groups.back().end = index - 1;
			groups.push_back({ node->tmp_flags, index, 0 });
			prev_flags = node->tmp_flags;
		}
		++index;
	}
	groups.back().end = index - 1;
}
