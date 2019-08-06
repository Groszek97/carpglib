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

SceneManager* app::scene_mgr;

SceneManager::SceneManager() : active_scene(nullptr), camera(nullptr), shader(nullptr), use_fog(true)
{
}

SceneManager::~SceneManager()
{
	DeleteElements(scenes);
	delete camera;
}

void SceneManager::Init()
{
	shader = new SuperShader();
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
	if(nodes.empty())
		return;

	V(device->BeginScene());

	Scene* scene = active_scene;
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

	shader->End();

	V(device->EndScene());
}

void SceneManager::SetActiveScene(Scene* scene)
{
	assert(!scene || IsInside(scenes, scene));
	active_scene = scene;
}
