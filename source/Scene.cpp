#include "EnginePch.h"
#include "EngineCore.h"
#include "Scene.h"
#include "SceneNode.h"
#include "MeshInstance.h"
#include "Camera.h"
#include "ResourceManager.h"

//=================================================================================================
Scene::~Scene()
{
	SceneNode::Free(nodes);
	SceneNode::Free(lights);
}

//=================================================================================================
void Scene::Add(SceneNode* node)
{
	assert(node);
	if(node->is_light)
		lights.push_back(node);
	else
		nodes.push_back(node);
}

//=================================================================================================
void Scene::Update(float dt)
{
	for(SceneNode* node : nodes)
	{
		if(node->mesh_inst)
			node->mesh_inst->Update(dt);
	}
}

//=================================================================================================
void Scene::ListNodes(Camera& camera, vector<SceneNode*>& visible_nodes, bool get_lights)
{
	FrustumPlanes frustum(camera.GetViewProjMatrix());

	for(SceneNode* node : nodes)
	{
		if(!node->mesh)
			continue;
		if(!node->mesh->IsLoaded())
			app::res_mgr->LoadInstant(node->mesh);
		if(frustum.SphereToFrustum(node->pos, node->mesh->head.radius))
		{
			if(node->mesh_inst)
				node->mesh_inst->SetupBones();

			node->mat = Matrix::Transform(node->pos, node->rot, node->scale);

			visible_nodes.push_back(node);

			if(get_lights)
				GetClosestLights(node);
		}
	}
}

//=================================================================================================
void Scene::GetClosestLights(SceneNode* node)
{
	constexpr int MAX_LIGHTS = 3;
	SceneNode* nodes[MAX_LIGHTS];
	float range[MAX_LIGHTS];
	for(int i = 0; i < MAX_LIGHTS; ++i)
	{
		nodes[i] = nullptr;
		range[i] = 1000000.f;
	}

	for(SceneNode* light : lights)
	{
		float dist = Vec3::DistanceSquared(node->pos, node->pos);
		for(int i = 0; i < MAX_LIGHTS; ++i)
		{
			if(range[i] > dist)
			{
				for(int j = MAX_LIGHTS - 1; j > i; --j)
				{
					range[j] = range[j - 1];
					nodes[j] = nodes[j - 1];
				}
				range[i] = dist;
				nodes[i] = light;
				break;
			}
		}
	}

	int count = 0;
	for(; count < MAX_LIGHTS; ++count)
	{
		if(!nodes[count])
			break;
	}

	node->lights.resize(count);
	if(count > 0)
		memcpy(node->lights.data(), nodes, sizeof(SceneNode*) * count);
}
