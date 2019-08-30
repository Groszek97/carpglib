#include "EnginePch.h"
#include "EngineCore.h"
#include "Scene.h"
#include "SceneNode.h"
#include "Camera.h"
#include "Mesh.h"

Scene::~Scene()
{
	SceneNode::Free(nodes);
}

void Scene::Update(float dt)
{

}

void Scene::ListVisibleNodes(Camera& camera, vector<SceneNode*>& visible_nodes)
{
	FrustumPlanes frustum(camera.GetViewProj());
	for(SceneNode* node : nodes)
	{
		if(node->mesh && node->visible && frustum.SphereToFrustum(node->pos, node->mesh->head.radius))
			visible_nodes.push_back(node);
	}
}
