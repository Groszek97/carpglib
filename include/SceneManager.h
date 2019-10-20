#pragma once

//-----------------------------------------------------------------------------
class SceneManager
{
public:
	SceneManager();
	~SceneManager();
	void Init();
	void Draw();
	void Update(float dt);
	void Add(Scene* scene) { assert(scene); scenes.push_back(scene); }
	void Add(Camera* camera) { assert(camera); cameras.push_back(camera); }
	Vec4 GetClearColor() const;
	void SetActive(Scene* scene) { this->scene = scene; }
	void SetActive(Camera* camera) { this->camera = camera; }

private:
	SuperShader* shader;
	vector<Scene*> scenes;
	vector<Camera*> cameras;
	Scene* scene;
	Camera* camera;
};
