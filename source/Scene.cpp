#include "EnginePch.h"
#include "EngineCore.h"
#include "Scene.h"
#include "SceneNode.h"
#include "Camera.h"
#include "MeshInstance.h"

Scene::~Scene()
{
	SceneNode::Free(nodes);
	SceneNode::Free(lights);
}

void Scene::Add(SceneNode* node)
{
	assert(node);
	if(node->is_light)
		lights.push_back(node);
	else
		nodes.push_back(node);
}

void Scene::Update(float dt)
{
	delegate<void(SceneNode*)> process = [dt, &process](SceneNode* node)
	{
		if(node->mesh_inst && node->own_mesh_inst)
			node->mesh_inst->Update(dt);
		if(!node->childs.empty())
		{
			for(SceneNode* child : node->childs)
				process(child);
		}
	};

	for(SceneNode* node : nodes)
		process(node);
}

void Scene::ListVisibleNodes(Camera& camera, vector<SceneNode*>& visible_nodes, bool get_lights)
{
	const Matrix& mat_view_proj = camera.GetViewProj();
	FrustumPlanes frustum(mat_view_proj);

	delegate<void(SceneNode*)> process = [&](SceneNode* node)
	{
		if(!node->mesh)
			return;
		if(!node->visible)
			return;
		node->mesh->EnsureIsLoaded();
		if(frustum.SphereToFrustum(node->pos, node->mesh->head.radius))
		{
			visible_nodes.push_back(node);

			if(node->mesh_inst)
				node->mesh_inst->SetupBones();

			if(node->billboard)
			{
				assert(!node->parent); // TODO
				node->mat = Matrix::CreateLookAt(node->pos, camera.from).Inverse() * mat_view_proj;
			}
			else
			{
				node->mat = Matrix::Transform(node->pos, node->rot, node->scale);
				if(node->parent)
				{
					if(node->point)
						node->mat = node->mat * node->point->mat * node->parent->mesh_inst->mat_bones[node->point->bone];
					node->mat *= node->parent->mat;
				}
			}

			if(get_lights)
				GetClosestLights(node);
			if(!node->childs.empty())
			{
				for(SceneNode* child : node->childs)
					process(child);
			}
		}
	};

	for(SceneNode* node : nodes)
		process(node);
}

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
