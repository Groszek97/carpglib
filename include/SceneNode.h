#pragma once

//-----------------------------------------------------------------------------
struct SceneNode : public ObjectPoolProxy<SceneNode>
{
	enum Flags
	{
		HAVE_WEIGHT = 1 << 0,
		HAVE_BINORMALS = 1 << 1,
		ANIMATED = 1 << 2,
		NORMAL_MAP = 1 << 3,
		SPECULAR_MAP = 1 << 4,
		TRANSPARENT = 1 << 5
	};

	void OnGet();
	void OnFree();
	void SetMesh(Mesh* mesh);
	void SetMesh(MeshInstance* mesh_inst);
	void SetLight(float range)
	{
		is_light = true;
		scale.x = range;
	}
	void ApplyMeshFlags();

	Matrix mat;
	Vec3 pos, rot, scale;
	Vec4 tint;
	Mesh* mesh;
	MeshInstance* mesh_inst;
	vector<SceneNode*> lights;
	int flags, tmp_flags;
	bool is_light;
};
