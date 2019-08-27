#pragma once

struct Scene
{
public:
	Scene() : clear_color(Color::Black), use_fog(false), use_point_light(false), use_dir_light(false) {}
	~Scene();
	void Add(SceneNode* node) { assert(node); nodes.push_back(node); }
	void Update(float dt);
	void ListVisibleNodes(Camera& camera, vector<SceneNode*>& visible_nodes);

	Color clear_color, fog_color, ambient_color, light_color;
	Vec2 fog_range;
	Vec3 light_dir;
	bool use_fog, use_point_light, use_dir_light;

private:
	vector<SceneNode*> nodes;
};
