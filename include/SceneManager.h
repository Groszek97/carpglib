#pragma once

class SceneManager
{
public:
	SceneManager();
	~SceneManager();
	void Init(Render* render);
	void Add(Scene* scene);
	Scene* CreateDefaultScene();
	void Draw();
	Scene* GetActiveScene() { return active_scene; }
	void SetActiveScene(Scene* scene);

private:
	Render* render;
	SuperShader* shader;
	vector<Scene*> scenes;
	vector<SceneNode*> nodes;
	Scene* active_scene;
};
