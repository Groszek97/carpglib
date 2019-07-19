#pragma once

#include "MeshInstance.h"

struct SceneNode
{
	SceneNode() : mesh(nullptr), mesh_inst(nullptr), pos(Vec3::Zero), rot(Vec3::Zero), scale(Vec3::One), changed(true)
	{
	}

	Matrix& GetWorldMatrix();
	void SetMesh(MeshInstance* mesh_inst)
	{
		this->mesh_inst = mesh_inst;
		mesh = mesh_inst->mesh;
	}

	Mesh* mesh;
	MeshInstance* mesh_inst;
	Vec3 pos, rot, scale;
	Matrix mat_world;
	bool changed;
};
