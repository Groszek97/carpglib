#include "EnginePch.h"
#include "EngineCore.h"
#include "SceneNode.h"
#include "MeshInstance.h"

void SceneNode::OnFree()
{
	delete mesh_inst;
}
