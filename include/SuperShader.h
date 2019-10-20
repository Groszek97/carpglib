#pragma once

//-----------------------------------------------------------------------------
class SuperShader
{
	enum Switches
	{
		HAVE_WEIGHT,
		HAVE_BINORMALS,
		ANIMATED,
		FOG,
		SPECULAR,
		NORMAL,
		POINT_LIGHT,
		DIR_LIGHT
	};

	struct Shader
	{
		uint id;
		ID3D11VertexShader* vertex_shader;
		ID3D11PixelShader* pixel_shader;
		ID3D11InputLayout* layout;
	};

	struct VertexGlobals
	{
		Matrix mat_combined;
		Matrix mat_world;
		Matrix mat_bones[32];
	};

public:
	SuperShader();
	~SuperShader();
	uint GetShaderId(bool have_weight, bool have_binormals, bool animated, bool fog, bool specular, bool normal, bool point_light, bool dir_light) const;
	void SetShader(uint id);
	void Prepare(Camera& camera);
	void Draw(SceneNode* node);

private:
	void Init();
	Shader& GetShader(uint id);
	Shader& CompileShader(uint id);

	ID3D11DeviceContext* device_context;
	ID3D11SamplerState* sampler_diffuse;
	ID3D11Buffer* vs_buffer;
	vector<Shader> shaders;
	Matrix mat_view_proj;
};
