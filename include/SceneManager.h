#pragma once

class SceneManager
{
public:
	SceneManager();
	~SceneManager();

	Scene* CreateDefaultScene();
	void Draw();

private:
	vector<Scene*> scenes;
	Scene* active_scene;
};
