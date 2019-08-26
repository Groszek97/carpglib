#pragma once

struct SceneNode : public ObjectPoolProxy<SceneNode>
{
	Mesh* mesh;
	Vec3 pos;
};
