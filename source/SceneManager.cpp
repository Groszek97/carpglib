#include "EnginePch.h"
#include "EngineCore.h"
#include "SceneManager.h"
#include "Render.h"
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

	Scene* scene = active_
	V(device->BeginScene());
}
