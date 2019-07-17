#pragma once

class Scene
{
public:
	Scene() : camera(nullptr), clear_color(Color::Black) {}
	~Scene();
	void Add(SceneNode* node) { nodes.push_back(node); }
	void GetVisibleNodes(vector<SceneNode*>& visible_nodes);
	Camera* GetCamera() { return camera; }
	Color GetClearColor() { return clear_color; }
	void SetCamera(Camera* camera) { this->camera = camera; }
	void SetClearColor(Color color) { clear_color = color; }

private:
	vector<SceneNode*> nodes;
	Camera* camera;
	Color clear_color;
};
