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

struct LightData
{
	Vec3 pos;
	float range;
	Vec4 color;
};

SceneManager::SceneManager() : active_scene(nullptr), active_camera(nullptr), shader(nullptr), use_fog(true), use_lighting(true), use_normal_map(true),
use_specular_map(true)
{
}

SceneManager::~SceneManager()
{
	DeleteElements(scenes);
	DeleteElements(cameras);
}

void SceneManager::Init()
{
	shader = new SuperShader;
	app::render->RegisterShader(shader);
	tex_specular = app::render->CreateTexture(Int2(1, 1), &Color::None);
	tex_normal = app::render->CreateTexture(Int2(1, 1), &Color(128, 128, 255));
}

void SceneManager::Draw()
{
	DrawInternal(active_scene, active_camera);
}

void SceneManager::Draw(RenderTarget* target, Scene* scene, Camera* camera)
{
	app::render->SetTarget(target);
	DrawInternal(scene, camera);
	app::render->SetTarget(nullptr);
}

void SceneManager::DrawInternal(Scene* scene, Camera* camera)
{
	IDirect3DDevice9* device = app::render->GetDevice();
	HRESULT hr = device->TestCooperativeLevel();
	if(hr != D3D_OK)
		return;

	if(!scene)
	{
		V(device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET | D3DCLEAR_STENCIL, Color::Black, 1.f, 0));
		return;
	}

	if(!camera)
	{
		V(device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET | D3DCLEAR_STENCIL, scene->clear_color, 1.f, 0));
		return;
	}

	app::render->SetAlphaBlend(false);
	app::render->SetAlphaTest(false);
	app::render->SetNoZWrite(false);
	app::render->SetNoCulling(false);

	V(device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET | D3DCLEAR_STENCIL, scene->clear_color, 1.f, 0));
	V(device->BeginScene());

	bool fog, point_light, dir_light;

	ID3DXEffect* effect = shader->GetEffect();

	V(effect->SetVector(shader->h_camera_pos, (D3DXVECTOR4*)& camera->from));

	// apply fog
	if(scene->use_fog && use_fog)
	{
		assert(scene->fog_range.x >= 0.f && scene->fog_range.x <= scene->fog_range.y);
		fog = true;

		Vec4 value = scene->fog_color;
		V(effect->SetVector(shader->h_fog_color, reinterpret_cast<D3DXVECTOR4*>(&value)));

		value = Vec4(scene->fog_range.x, scene->fog_range.y, scene->fog_range.y - scene->fog_range.x, 0.f);
		V(effect->SetVector(shader->h_fog_params, reinterpret_cast<D3DXVECTOR4*>(&value)));
	}
	else
		fog = false;

	// apply lighting
	if(use_lighting && (scene->use_point_light || scene->use_dir_light))
	{
		Vec4 value = scene->ambient_color;
		V(effect->SetVector(shader->h_ambient_color, reinterpret_cast<D3DXVECTOR4*>(&value)));

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
			V(effect->SetVector(shader->h_light_color, reinterpret_cast<D3DXVECTOR4*>(&value)));

			value = Vec4(scene->light_dir.Normalized(), 1.f);
			V(effect->SetVector(shader->h_light_dir, reinterpret_cast<D3DXVECTOR4*>(&value)));
		}
	}
	else
	{
		point_light = false;
		dir_light = false;
	}

	nodes.clear();
	scene->ListVisibleNodes(*camera, nodes, point_light);
	ProcessNodes(camera);

	const Matrix& mat_view_proj = camera->GetViewProj();
	Matrix mat_world;
	Mesh* mesh = nullptr;
	LightData lights[3];
	memset(lights, 0, sizeof(lights));
	uint passes;

	// non transparent nodes
	for(SceneNodeGroup& group : groups)
	{
		const bool animated = IsSet(group.flags, SceneNode::ANIMATED);
		const bool normal_map = IsSet(group.flags, SceneNode::NORMAL_MAP);
		const bool specular_map = IsSet(group.flags, SceneNode::SPECULAR_MAP);

		uint flags = shader->GetShaderId(animated, IsSet(group.flags, SceneNode::HAVE_BINORMALS),
			fog, specular_map, normal_map, point_light, dir_light);

		effect = shader->GetShader(flags);
		D3DXHANDLE tech;
		V(effect->FindNextValidTechnique(nullptr, &tech));
		V(effect->SetTechnique(tech));

		V(effect->Begin(&passes, 0));
		V(effect->BeginPass(0));

		for(auto it = non_transparent.begin() + group.start, end = non_transparent.begin() + group.end + 1; it != end; ++it)
		{
			SceneNode* node = *it;
			Matrix mat_combined = node->mat * mat_view_proj;

			// set mesh
			if(node->mesh != mesh)
			{
				mesh = node->mesh;
				V(device->SetVertexDeclaration(app::render->GetVertexDeclaration(mesh->vertex_decl)));
				V(device->SetStreamSource(0, mesh->vb, 0, mesh->vertex_size));
				V(device->SetIndices(mesh->ib));
			}

			V(effect->SetMatrix(shader->h_mat_combined, reinterpret_cast<D3DXMATRIX*>(&mat_combined)));
			V(effect->SetMatrix(shader->h_mat_world, reinterpret_cast<D3DXMATRIX*>(&node->mat)));
			V(effect->SetVector(shader->h_tint, reinterpret_cast<D3DXVECTOR4*>(&node->tint)));
			if(animated)
			{
				MeshInstance& mesh_inst = *node->mesh_inst;
				V(effect->SetMatrixArray(shader->h_mat_bones, (D3DXMATRIX*)mesh_inst.mat_bones.data(), mesh_inst.mat_bones.size()));
			}
			if(point_light)
			{
				for(uint i = 0, count = min(3u, node->lights.size()); i < count; ++i)
				{
					lights[i].pos = node->lights[i]->pos;
					lights[i].range = node->lights[i]->scale.x;
					lights[i].color = node->lights[i]->tint;
				}
				V(effect->SetRawValue(shader->h_lights, lights, 0, sizeof(LightData) * 3));
			}

			for(int i = 0; i < mesh->head.n_subs; ++i)
			{
				if(!IsSet(node->subs, 1 << i))
					continue;

				const Mesh::Submesh& sub = mesh->subs[i];

				// set texture
				if(node->tex)
				{
					const TexOverride& tex = node->tex[i];
					V(effect->SetTexture(shader->h_tex_diffuse, tex.diffuse->tex));
					if(normal_map)
						V(effect->SetTexture(shader->h_tex_normal, tex.normal ? tex.normal->tex : tex_normal));
					if(specular_map)
						V(effect->SetTexture(shader->h_tex_specular, tex.specular ? tex.specular->tex : tex_specular));
				}
				else
				{
					V(effect->SetTexture(shader->h_tex_diffuse, mesh->GetTexture(i)));
					if(normal_map)
						V(effect->SetTexture(shader->h_tex_normal, sub.tex_normal ? sub.tex_normal->tex : tex_normal));
					if(specular_map)
						V(effect->SetTexture(shader->h_tex_specular, sub.tex_specular ? sub.tex_specular->tex : tex_specular));
				}

				// lighting
				V(effect->SetVector(shader->h_specular_color, reinterpret_cast<const D3DXVECTOR4*>(&sub.specular_color)));
				V(effect->SetFloat(shader->h_specular_intensity, sub.specular_intensity));
				V(effect->SetFloat(shader->h_specular_hardness, (float)sub.specular_hardness));

				V(effect->CommitChanges());
				V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sub.min_ind, sub.n_ind, sub.first * 3, sub.tris));
			}
		}

		V(effect->EndPass());
		V(effect->End());
	}

	// transparent nodes
	if(!transparent.empty())
	{
		app::render->SetAlphaBlend(true);
		//app::render->GetDevice()->SetRenderState(D3DRS_Z)
		//app::render->SetNoZWrite(true);
		uint prev_flags = -1;
		bool open = false;
		for(SceneNode* node : transparent)
		{
			const bool animated = IsSet(node->tmp_flags, SceneNode::ANIMATED);
			const bool specular_map = IsSet(node->tmp_flags, SceneNode::SPECULAR_MAP);
			const bool normal_map = IsSet(node->tmp_flags, SceneNode::NORMAL_MAP);
			const uint flags = shader->GetShaderId(animated, IsSet(node->flags, SceneNode::HAVE_BINORMALS),
				fog, specular_map, normal_map, point_light, dir_light);
			if(flags != prev_flags)
			{
				prev_flags = flags;
				if(open)
				{
					V(effect->EndPass());
					V(effect->End());
				}
				open = true;

				effect = shader->GetShader(flags);
				D3DXHANDLE tech;
				V(effect->FindNextValidTechnique(nullptr, &tech));
				V(effect->SetTechnique(tech));

				V(effect->Begin(&passes, 0));
				V(effect->BeginPass(0));
			}

			Matrix mat_combined = node->mat * mat_view_proj;

			// set mesh
			if(node->mesh != mesh)
			{
				mesh = node->mesh;
				V(device->SetVertexDeclaration(app::render->GetVertexDeclaration(mesh->vertex_decl)));
				V(device->SetStreamSource(0, mesh->vb, 0, mesh->vertex_size));
				V(device->SetIndices(mesh->ib));
			}

			V(effect->SetMatrix(shader->h_mat_combined, reinterpret_cast<D3DXMATRIX*>(&mat_combined)));
			V(effect->SetMatrix(shader->h_mat_world, reinterpret_cast<D3DXMATRIX*>(&node->mat)));
			V(effect->SetVector(shader->h_tint, reinterpret_cast<D3DXVECTOR4*>(&node->tint)));
			if(animated)
			{
				MeshInstance& mesh_inst = *node->mesh_inst;
				V(effect->SetMatrixArray(shader->h_mat_bones, (D3DXMATRIX*)mesh_inst.mat_bones.data(), mesh_inst.mat_bones.size()));
			}
			if(point_light)
			{
				for(uint i = 0, count = min(3u, node->lights.size()); i < count; ++i)
				{
					lights[i].pos = node->lights[i]->pos;
					lights[i].range = node->lights[i]->scale.x;
					lights[i].color = node->lights[i]->tint;
				}
				V(effect->SetRawValue(shader->h_lights, lights, 0, sizeof(LightData) * 3));
			}

			for(int i = 0; i < mesh->head.n_subs; ++i)
			{
				if(!IsSet(node->subs, 1 << i))
					continue;

				const Mesh::Submesh& sub = mesh->subs[i];

				// set texture
				if(node->tex)
				{
					const TexOverride& tex = node->tex[i];
					V(effect->SetTexture(shader->h_tex_diffuse, tex.diffuse->tex));
					if(normal_map)
						V(effect->SetTexture(shader->h_tex_normal, tex.normal ? tex.normal->tex : tex_normal));
					if(specular_map)
						V(effect->SetTexture(shader->h_tex_specular, tex.specular ? tex.specular->tex : tex_specular));
				}
				else
				{
					V(effect->SetTexture(shader->h_tex_diffuse, mesh->GetTexture(i)));
					if(normal_map)
						V(effect->SetTexture(shader->h_tex_normal, sub.tex_normal ? sub.tex_normal->tex : tex_normal));
					if(specular_map)
						V(effect->SetTexture(shader->h_tex_specular, sub.tex_specular ? sub.tex_specular->tex : tex_specular));
				}

				// lighting
				V(effect->SetVector(shader->h_specular_color, reinterpret_cast<const D3DXVECTOR4*>(&sub.specular_color)));
				V(effect->SetFloat(shader->h_specular_intensity, sub.specular_intensity));
				V(effect->SetFloat(shader->h_specular_hardness, (float)sub.specular_hardness));

				V(effect->CommitChanges());
				V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sub.min_ind, sub.n_ind, sub.first * 3, sub.tris));
			}
		}

		V(effect->EndPass());
		V(effect->End());
	}

	V(device->EndScene());
}

void SceneManager::ProcessNodes(Camera* camera)
{
	groups.clear();
	transparent.clear();
	non_transparent.clear();

	if(nodes.empty())
		return;

	int flag_filter = SceneNode::ANIMATED | SceneNode::HAVE_BINORMALS | SceneNode::TRANSPARENT;
	if(use_normal_map)
		flag_filter |= SceneNode::NORMAL_MAP;
	if(use_specular_map)
		flag_filter |= SceneNode::SPECULAR_MAP;

	for(SceneNode* node : nodes)
	{
		node->tmp_flags = node->flags & flag_filter;
		if(IsSet(node->tmp_flags, SceneNode::TRANSPARENT))
		{
			node->dist = Vec3::DistanceSquared(node->pos, camera->from);
			transparent.push_back(node);
		}
		else
			non_transparent.push_back(node);
	}

	// sort non transparent nodes
	std::sort(non_transparent.begin(), non_transparent.end(), [](const SceneNode* node1, const SceneNode* node2)
	{
		if(node1->tmp_flags != node2->tmp_flags)
			return node1->tmp_flags > node2->tmp_flags;
		else
			return node1->mesh > node2->mesh;
	});

	int prev_flags = -1, index = 0;
	for(SceneNode* node : non_transparent)
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

	// sort transparent nodes
	std::sort(transparent.begin(), transparent.end(), [](const SceneNode* node1, const SceneNode* node2)
	{
		return node1->dist > node2->dist;
	});
}

void SceneManager::Update(float dt)
{
	for(Scene* scene : scenes)
		scene->Update(dt);
}
