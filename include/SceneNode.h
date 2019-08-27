#pragma once

struct SceneNode : public ObjectPoolProxy<SceneNode>
{
	SceneNode() : mesh(nullptr), mesh_inst(nullptr) {}

	Mesh* mesh;
	MeshInstance* mesh_inst;
	Matrix mat_world;
	Vec3 pos, rot, scale;
	bool changed;
};
