#pragma once

class SceneManager
{
public:
	SceneManager();
	~SceneManager();
	void Init();
	void Add(Scene* scene);
	void Draw();
	Scene* GetActiveScene() { return active_scene; }
	Camera* GetCamera() { return camera; }
	void SetActiveScene(Scene* scene);
	void SetCamera(Camera* camera) { this->camera = camera; }

	bool use_fog;

private:
	SuperShader* shader;
	vector<Scene*> scenes;
	vector<SceneNode*> nodes;
	Scene* active_scene;
	Camera* camera;
};
