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

	Camera* GetActiveCamera() const { return camera; }
	Scene* GetActiveScene() const { return scene; }
	Vec4 GetClearColor() const;

	void SetActive(Scene* scene) { this->scene = scene; }
	void SetActive(Camera* camera) { this->camera = camera; }

	bool fog_enabled, lighting_enabled, normal_map_enabled, specular_map_enabled;

private:
	struct SceneNodeGroup
	{
		int flags, start, end;
	};

	void ProcessNodes();

	SuperShader* shader;
	vector<Scene*> scenes;
	vector<Camera*> cameras;
	vector<SceneNode*> visible_nodes;
	vector<SceneNodeGroup> groups;
	Scene* scene;
	Camera* camera;
};
