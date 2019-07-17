#pragma once

struct SceneNode
{
	SceneNode() : mesh(nullptr), pos(Vec3::Zero), rot(Vec3::Zero), scale(Vec3::One), changed(true)
	{
	}

	Matrix& GetWorldMatrix();

	Mesh* mesh;
	Vec3 pos, rot, scale;
	Matrix mat_world;
	bool changed;
};
