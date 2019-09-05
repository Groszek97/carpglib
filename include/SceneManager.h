#pragma once

class SceneManager
{
public:
	SceneManager() : active_scene(nullptr), use_fog(true), use_lighting(true), use_normal_map(true), use_specular_map(true) {}

	void Draw();

	bool use_fog, use_lighting, use_normal_map, use_specular_map;

private:
	struct SceneNodeGroup
	{
		int flags, start, end;
	};

	void ProcessNodes();

	Camera* camera;
	Scene* active_scene;
	SuperShader* shader;
	vector<SceneNode*> nodes;
	vector<SceneNodeGroup> groups;
};
