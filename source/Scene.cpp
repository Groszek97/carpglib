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
void Scene::ListNodes(Camera& camera, vector<SceneNode*>& visible_nodes)
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
		}
	}
}
