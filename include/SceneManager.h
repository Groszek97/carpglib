#pragma once

class SceneManager
{
public:
	SceneManager();
	~SceneManager();
	void Init(Render* render);
	Scene* CreateDefaultScene();
	void Draw();

private:
	Render* render;
	std::unique_ptr<SuperShader> shader;
	vector<Scene*> scenes;
	vector<SceneNode*> nodes;
	Scene* active_scene;
};
