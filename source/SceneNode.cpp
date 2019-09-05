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
	ApplyMeshFlags();
}

void SceneNode::SetMesh(MeshInstance* mesh_inst)
{
	assert(mesh_inst);
	this->mesh = mesh_inst->mesh;
	this->mesh_inst = mesh_inst;
	ApplyMeshFlags();
}

void SceneNode::ApplyMeshFlags()
{
	flags = 0;
	if(IsSet(mesh->head.flags, Mesh::F_ANIMATED))
		flags |= ANIMATED;
	if(IsSet(mesh->head.flags, Mesh::F_TANGENTS))
		flags |= HAVE_BINORMALS;
	if(IsSet(mesh->head.flags, Mesh::F_NORMAL_MAP))
		flags |= NORMAL_MAP;
	if(IsSet(mesh->head.flags, Mesh::F_SPECULAR_MAP))
		flags |= SPECULAR_MAP;
}
