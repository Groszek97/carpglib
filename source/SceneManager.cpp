#include "EnginePch.h"
#include "EngineCore.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Render.h"
#include "SuperShader.h"
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

	V(device->BeginScene());
	V(device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET | D3DCLEAR_STENCIL, scene->clear_color, 1.f, 0));

	if(scene->use_fog && use_fog)
		shader->SetFog(scene->fog_color, scene->fog_range);
	else
		shader->SetFogDisabled();

	if(use_lighting && (scene->use_point_light || scene->use_dir_light))
	{
		if(scene->use_point_light)
			shader->SetPointLight(scene->ambient_color);
		else
			shader->SetDirLight(scene->ambient_color, scene->light_color, scene->light_dir);
	}
	else
		shader->SetLightingDisabled();

	V(device->EndScene());
}
