#pragma once

struct Scene
{
public:
	~Scene();
	void Add(SceneNode* node) { assert(node); nodes.push_back(node); }
	void Update(float dt);
	void ListVisibleNodes(Camera& camera, vector<SceneNode*>& visible_nodes);

private:
	vector<SceneNode*> nodes;
};
