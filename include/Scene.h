#pragma once

//-----------------------------------------------------------------------------
struct Scene
{
	Scene() : clear_color(Color::Black) {}
	~Scene();
	void Add(SceneNode* node) { nodes.push_back(node); }

	vector<SceneNode*> nodes;
	Color clear_color;
};
