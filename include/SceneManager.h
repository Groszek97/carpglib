#pragma once

class SceneManager
{
public:
	typedef delegate<void(DebugDrawer*)> DrawHandler;

	SceneManager();
	~SceneManager();
	void Init();
	void Add(Scene* scene);
	void AddHandler(DrawHandler handler) { handlers.push_back(handler); }
	void RemoveHandler(DrawHandler handler) { RemoveElement(handlers, handler); }
	void Draw();
	Scene* GetActiveScene() { return active_scene; }
	Camera* GetCamera() { return camera; }
	void SetActiveScene(Scene* scene);
	void SetCamera(Camera* camera) { this->camera = camera; }

	bool use_fog, use_lighting;

private:
	void DrawNodes();

	SuperShader* shader;
	DebugDrawer* debug_drawer;
	vector<Scene*> scenes;
	vector<SceneNode*> nodes;
	vector<DrawHandler> handlers;
	Scene* active_scene;
	Camera* camera;
};
