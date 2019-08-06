#include "EnginePch.h"
#include "EngineCore.h"
#include "Scene.h"
#include "SceneNode.h"
#include "Camera.h"

Scene::Scene() : clear_color(Color::Black), use_fog(true), fog_range(20.f, 40.f), fog_color(Color::Black)
{
}

Scene::~Scene()
{
	DeleteElements(nodes);
}

void Scene::Update(float dt)
{
	for(SceneNode* node : nodes)
	{
		if(node->mesh_inst)
			node->mesh_inst->Update(dt);
	}
}

void Scene::GetVisibleNodes(vector<SceneNode*>& visible_nodes)
{
	for(SceneNode* node : nodes)
		visible_nodes.push_back(node);
}
