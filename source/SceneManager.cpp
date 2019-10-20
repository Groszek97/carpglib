#include "EnginePch.h"
#include "EngineCore.h"
#include "SceneManager.h"
#include "Scene.h"
#include "SceneNode.h"
#include "Camera.h"
#include "MeshInstance.h"
#include "SuperShader.h"

SceneManager* app::scene_mgr;

SceneManager::SceneManager() : scene(nullptr), camera(nullptr), shader(nullptr)
{
}

SceneManager::~SceneManager()
{
	DeleteElements(scenes);
	DeleteElements(cameras);
	delete shader;
}

void SceneManager::Init()
{
	shader = new SuperShader;
}

void SceneManager::Draw()
{
	if(!scene || !camera)
		return;

	shader->Prepare(*camera);
	for(SceneNode* node : scene->nodes)
	{
		uint id = shader->GetShaderId(IsSet(node->mesh->head.flags, Mesh::F_ANIMATED), IsSet(node->mesh->head.flags, Mesh::F_TANGENTS),
			node->mesh_inst != nullptr, false, false, false, false, false);
		shader->SetShader(id);
		shader->Draw(node);
	}
}

void SceneManager::Update(float dt)
{
	for(SceneNode* node : scene->nodes)
	{
		if(node->mesh_inst)
			node->mesh_inst->Update(dt);
	}
}

Vec4 SceneManager::GetClearColor() const
{
	return (Vec4)(scene ? scene->clear_color : Color::Black);
}
