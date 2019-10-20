#pragma once

//-----------------------------------------------------------------------------
struct SceneNode : public ObjectPoolProxy<SceneNode>
{
	void OnFree();

	Vec3 pos, rot;
	Mesh* mesh;
	MeshInstance* mesh_inst;
};
