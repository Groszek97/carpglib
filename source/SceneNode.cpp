#include "EnginePch.h"
#include "EngineCore.h"
#include "SceneNode.h"
#include "MeshInstance.h"

SceneNode::~SceneNode()
{
	delete mesh_inst;
}

void SceneNode::SetMesh(Mesh* mesh)
{
	assert(mesh);
	this->mesh = mesh;
}

void SceneNode::SetMesh(MeshInstance* mesh_inst)
{
	assert(mesh_inst);
	this->mesh = mesh_inst->mesh;
	this->mesh_inst = mesh_inst;
}
