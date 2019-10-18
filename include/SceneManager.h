#pragma once

//-----------------------------------------------------------------------------
class SceneManager
{
public:
	SceneManager();
	~SceneManager();
	void Init();
	void Draw();
	void Add(Scene* scene) { assert(scene); scenes.push_back(scene); }
	void Add(Camera* camera) { assert(camera); cameras.push_back(camera); }
	void SetActive(Scene* scene) { this->scene = scene; }
	void SetActive(Camera* camera) { this->camera = camera; }

private:
	vector<Scene*> scenes;
	vector<Camera*> cameras;
	Scene* scene;
	Camera* camera;

	ID3D11VertexShader* vertex_shader;
	ID3D11PixelShader* pixel_shader;
	ID3D11InputLayout* layout;
	ID3D11Buffer* vs_buffer;
	ID3D11SamplerState* sampler;
};
