#pragma once

struct SceneNode : public ObjectPoolProxy<SceneNode>
{
	friend class SceneManager;

	SceneNode() : mesh(nullptr), mesh_inst(nullptr), changed(true), visible(true), is_light(false) {}
	~SceneNode();
	void SetMesh(Mesh* mesh);
	void SetMesh(MeshInstance* mesh_inst);

private:
	Mesh* mesh;
	MeshInstance* mesh_inst;
	Matrix mat_world;
	Vec3 pos, rot, scale;
	bool changed, visible, is_light;
};
