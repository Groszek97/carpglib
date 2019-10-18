#pragma once

//-----------------------------------------------------------------------------
struct SceneNode : public ObjectPoolProxy<SceneNode>
{
	Vec3 pos, rot;
	Mesh* mesh;
};
