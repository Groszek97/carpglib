#include "EnginePch.h"
#include "EngineCore.h"
#include "Scene.h"
#include "SceneNode.h"

//=================================================================================================
Scene::~Scene()
{
	SceneNode::Free(nodes);
}
