#pragma once

class SceneManager
{
public:
	SceneManager() : active_scene(nullptr), use_fog(true) {}

	void Draw();

	bool use_fog, use_lighting;

private:
	Camera* camera;
	Scene* active_scene;
	SuperShader* shader;
	vector<SceneNode*> nodes;
};