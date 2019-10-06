#pragma once

class SceneManager
{
public:
	SceneManager();
	~SceneManager();

	void Init();
	void Draw();
	void Update(float dt);
	void AddScene(Scene* scene)
	{
		assert(scene);
		scenes.push_back(scene);
	}
	void SetActiveScene(Scene* scene);
	void SetCamera(Camera* camera) { this->camera = camera; }

	bool use_fog, use_lighting, use_normal_map, use_specular_map;

private:
	struct SceneNodeGroup
	{
		int flags, start, end;
	};

	void ProcessNodes();

	Camera* camera;
	vector<Scene*> scenes;
	Scene* active_scene;
	SuperShader* shader;
	vector<SceneNode*> nodes;
	vector<SceneNodeGroup> groups;
	TEX tex_specular, tex_normal;
};
