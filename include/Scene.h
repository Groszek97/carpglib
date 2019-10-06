#pragma once

struct Scene
{
public:
	Scene() : clear_color(Color::Black), use_fog(false), use_point_light(false), use_dir_light(false) {}
	~Scene();
	void Add(SceneNode* node);
	void Update(float dt);
	void ListVisibleNodes(Camera& camera, vector<SceneNode*>& visible_nodes, bool get_lights);

	Color clear_color, fog_color, ambient_color, light_color;
	Vec2 fog_range;
	Vec3 light_dir;
	bool use_fog, use_point_light, use_dir_light;

private:
	void GetClosestLights(SceneNode* node);

	vector<SceneNode*> nodes;
	vector<SceneNode*> lights;
};
