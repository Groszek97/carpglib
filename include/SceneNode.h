#pragma once

struct SceneNode : public ObjectPoolProxy<SceneNode>
{
	enum Flags
	{
		ANIMATED = 1 << 0,
		HAVE_BINORMALS = 1 << 1
	};

	friend class SceneManager;

	SceneNode() : mesh(nullptr), mesh_inst(nullptr), changed(true), visible(true), is_light(false) {}
	~SceneNode();
	void SetMesh(Mesh* mesh);
	void SetMesh(MeshInstance* mesh_inst);
	const Matrix& GetWorldMatrix();

//private:
	Mesh* mesh;
	MeshInstance* mesh_inst;
	Matrix mat_world;
	Vec3 pos, rot, scale;
	int flags, tmp_flags;
	bool changed, visible, is_light;
};
