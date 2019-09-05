#pragma once

struct SceneNode : public ObjectPoolProxy<SceneNode>
{
	enum Flags
	{
		ANIMATED = 1 << 0,
		HAVE_BINORMALS = 1 << 1,
		NORMAL_MAP = 1 << 2,
		SPECULAR_MAP = 1 << 3
	};

	friend class SceneManager;

	SceneNode() : mesh(nullptr), mesh_inst(nullptr), visible(true), is_light(false), billboard(false), subs(-1) {}
	~SceneNode();
	void SetMesh(Mesh* mesh);
	void SetMesh(MeshInstance* mesh_inst);

//private:
	void ApplyMeshFlags();

	vector<SceneNode*> childs, lights;
	Mesh* mesh;
	MeshInstance* mesh_inst;
	Vec4 tint;
	Vec3 pos, rot, scale;
	int flags, tmp_flags, subs;
	bool visible, is_light, billboard;
};
