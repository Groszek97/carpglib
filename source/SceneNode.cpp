#include "EnginePch.h"
#include "EngineCore.h"
#include "SceneNode.h"
#include "MeshInstance.h"

//=================================================================================================
void SceneNode::OnGet()
{
	type = NORMAL;
	mesh = nullptr;
	mesh_inst = nullptr;
	pos = Vec3::Zero;
	rot = Vec3::Zero;
	scale = Vec3::One;
	tint = Vec4::One;
}

//=================================================================================================
void SceneNode::OnFree()
{
	delete mesh_inst;
}

//=================================================================================================
void SceneNode::SetMesh(Mesh* mesh)
{
	assert(mesh);
	this->mesh = mesh;
	ApplyMeshFlags();
}

//=================================================================================================
void SceneNode::SetMesh(MeshInstance* mesh_inst)
{
	assert(mesh_inst);
	this->mesh = mesh_inst->mesh;
	this->mesh_inst = mesh_inst;
	ApplyMeshFlags();
}

//=================================================================================================
void SceneNode::ApplyMeshFlags()
{
	flags = 0;
	if(IsSet(mesh->head.flags, Mesh::F_ANIMATED))
		flags |= HAVE_WEIGHT;
	if(IsSet(mesh->head.flags, Mesh::F_TANGENTS))
		flags |= HAVE_BINORMALS;
	if(mesh_inst && IsSet(mesh->head.flags, Mesh::F_ANIMATED))
		flags |= ANIMATED;
	if(IsSet(mesh->head.flags, Mesh::F_NORMAL_MAP))
		flags |= NORMAL_MAP;
	if(IsSet(mesh->head.flags, Mesh::F_SPECULAR_MAP))
		flags |= SPECULAR_MAP;
}
