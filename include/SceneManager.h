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
	SuperShader* shader;
	vector<Scene*> scenes;
	vector<SceneNode*> nodes;
	Scene* active_scene;
};
