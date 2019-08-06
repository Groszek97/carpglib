#pragma once

class Scene
{
public:
	Scene() : clear_color(Color::Black) {}
	~Scene();
	void Add(SceneNode* node) { nodes.push_back(node); }
	void Update(float dt);
	void GetVisibleNodes(vector<SceneNode*>& visible_nodes);

	Color clear_color;

private:
	vector<SceneNode*> nodes;
};
