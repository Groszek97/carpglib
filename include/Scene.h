#pragma once

class Scene
{
public:
	Scene();
	~Scene();
	void Add(SceneNode* node) { nodes.push_back(node); }
	void Update(float dt);
	void GetVisibleNodes(vector<SceneNode*>& visible_nodes);

	Color clear_color;
	Color ambient_color;
	Color light_color;
	Vec3 light_dir;
	Vec2 fog_range;
	Color fog_color;
	bool use_fog, use_light;

private:
	vector<SceneNode*> nodes;
};
