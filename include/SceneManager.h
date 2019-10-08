#pragma once

class SceneManager
{
public:
	SceneManager();
	~SceneManager();

	void Init();
	void Draw();
	void Draw(RenderTarget* target, Scene* scene, Camera* camera);
	void Update(float dt);
	void AddScene(Scene* scene)
	{
		assert(scene);
		scenes.push_back(scene);
	}
	void AddCamera(Camera* camera)
	{
		assert(camera);
		cameras.push_back(camera);
	}
	void SetActiveScene(Scene* scene) { active_scene = scene; }
	void SetActiveCamera(Camera* camera) { active_camera = camera; }

	bool use_fog, use_lighting, use_normal_map, use_specular_map;

private:
	struct SceneNodeGroup
	{
		int flags, start, end;
	};

	void DrawInternal(Scene* scene, Camera* camera);
	void ProcessNodes(Camera* camera);

	Scene* active_scene;
	Camera* active_camera;
	vector<Scene*> scenes;
	vector<Camera*> cameras;
	SuperShader* shader;
	vector<SceneNode*> nodes, non_transparent, transparent;
	vector<SceneNodeGroup> groups;
	TEX tex_specular, tex_normal;
};
