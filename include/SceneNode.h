#pragma once

#include "Mesh.h"

struct TexOverride
{
	TexturePtr diffuse, specular, normal;
};

struct SceneNode : public ObjectPoolProxy<SceneNode>
{
	enum Flags
	{
		ANIMATED = 1 << 0,
		HAVE_BINORMALS = 1 << 1,
		NORMAL_MAP = 1 << 2,
		SPECULAR_MAP = 1 << 3,
		TRANSPARENT = 1 << 4
	};

	friend class SceneManager;

	void OnGet();
	void OnFree();
	void SetMesh(Mesh* mesh);
	void SetMesh(MeshInstance* mesh_inst);
	void SetLight(float range)
	{
		is_light = true;
		scale.x = range;
	}
	void SetTexture(TexOverride* tex);
	void AddChild(SceneNode* node, Mesh::Point* point = nullptr, bool use_parent_mesh = false);

private:
	void ApplyMeshFlags();

public:
	SceneNode* parent;
	vector<SceneNode*> childs, lights;
	Mesh* mesh;
	MeshInstance* mesh_inst;
	TexOverride* tex;
	Mesh::Point* point;
	Matrix mat;
	Vec4 tint;
	Vec3 pos, rot, scale;
	float dist;
	int flags, tmp_flags, subs;
	bool visible, is_light, billboard, own_mesh_inst;
};
