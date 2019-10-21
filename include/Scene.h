#pragma once

//-----------------------------------------------------------------------------
struct Scene
{
	Scene() : clear_color(Color::Black), ambient_color(0.4f, 0.4f, 0.4f), use_fog(false) {}
	~Scene();
	void Add(SceneNode* node) { nodes.push_back(node); }
	void Update(float dt);
	void ListNodes(Camera& camera, vector<SceneNode*>& visible_nodes);

	vector<SceneNode*> nodes;
	Color clear_color, fog_color, ambient_color;
	Vec2 fog_range;
	bool use_fog;
};
