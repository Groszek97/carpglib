#include "EnginePch.h"
#include "EngineCore.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Camera.h"

SceneManager::SceneManager() : active_scene(nullptr)
{
}

SceneManager::~SceneManager()
{
	DeleteElements(scenes);
}

Scene* SceneManager::CreateDefaultScene()
{
	assert(!active_scene);
	active_scene = new Scene;
	active_scene->SetCamera(new Camera);
	scenes.push_back(active_scene);
}

void SceneManager::Draw()
{

}
