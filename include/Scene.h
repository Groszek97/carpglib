#pragma once

class Scene
{
public:
	Scene() : camera(nullptr) {}
	void Add(SceneNode* node) { nodes.push_back(node); }
	Camera* GetCamera() { return camera; }
	void SetCamera(Camera* camera) { this->camera = camera; }

private:
	vector<SceneNode*> nodes;
	Camera* camera;
};
