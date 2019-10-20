#pragma once

//-----------------------------------------------------------------------------
struct Scene
{
	Scene() : clear_color(Color::Black), use_fog(false) {}
	~Scene();
	void Add(SceneNode* node) { nodes.push_back(node); }

	vector<SceneNode*> nodes;
	Color clear_color, fog_color;
	Vec2 fog_range;
	bool use_fog;
};
