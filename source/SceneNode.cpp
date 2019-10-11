#include "EnginePch.h"
#include "EngineCore.h"
#include "SceneNode.h"
#include "MeshInstance.h"

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
	own_mesh_inst = true;
	ApplyMeshFlags();
}

void SceneNode::SetTexture(TexOverride* tex)
{
	this->tex = tex;
	ApplyMeshFlags();
}

void SceneNode::AddChild(SceneNode* node, Mesh::Point* point, bool use_parent_mesh)
{
	assert(node && !node->parent);
	if(point)
	{
		assert(mesh_inst);
		assert(mesh->HavePoint(point));
	}
	if(use_parent_mesh)
	{
		assert(mesh_inst && !node->mesh_inst && node->mesh); // SetMesh before AddChild!
		node->mesh_inst = mesh_inst;
		node->own_mesh_inst = false;
		node->flags |= ANIMATED;
	}
	node->parent = this;
	node->point = point;
	childs.push_back(node);
}

SceneNode* SceneNode::GetChild(int id)
{
	for(SceneNode* child : childs)
	{
		if(child->id == id)
			return child;
	}
	return nullptr;
}

void SceneNode::ApplyMeshFlags()
{
	flags = 0;
	if(IsSet(mesh->head.flags, Mesh::F_ANIMATED) && mesh_inst)
		flags |= ANIMATED;
	if(IsSet(mesh->head.flags, Mesh::F_TANGENTS))
		flags |= HAVE_BINORMALS;
	if(!tex)
	{
		if(IsSet(mesh->head.flags, Mesh::F_NORMAL_MAP))
			flags |= NORMAL_MAP;
		if(IsSet(mesh->head.flags, Mesh::F_SPECULAR_MAP))
			flags |= SPECULAR_MAP;
	}
	else
	{
		for(int i = 0; i < mesh->head.n_subs; ++i)
		{
			if(tex[i].normal)
				flags |= NORMAL_MAP;
			if(tex[i].specular)
				flags |= SPECULAR_MAP;
		}
	}
	if(tint.w != 1.f)
		flags |= TRANSPARENT;
}

void SceneNode::OnGet()
{
	parent = nullptr;
	point = nullptr;
	mesh = nullptr;
	mesh_inst = nullptr;
	tex = nullptr;
	visible = true;
	is_light = false;
	billboard = false;
	subs = -1;
	pos = Vec3::Zero;
	rot = Vec3::Zero;
	scale = Vec3::One;
	tint = Vec4::One;
}

void SceneNode::OnFree()
{
	Free(childs);
	if(mesh_inst && own_mesh_inst)
		delete mesh_inst;
}
