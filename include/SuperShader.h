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

	struct PixelGlobals
	{
		Vec4 fog_color;
		Vec4 fog_params;
	};

public:
	SuperShader();
	~SuperShader();
	uint GetShaderId(bool have_weight, bool have_binormals, bool animated, bool fog, bool specular, bool normal, bool point_light, bool dir_light) const;
	void SetShader(uint id);
	void SetFog(Color color, const Vec2& range)
	{
		assert(range.x >= 0 && range.y >= range.x);
		fog_color = color;
		fog_range = range;
	}
	void Prepare(Camera& camera);
	void Draw(SceneNode* node);

private:
	void Init();
	Shader& GetShader(uint id);
	Shader& CompileShader(uint id);

	ID3D11DeviceContext* device_context;
	ID3D11SamplerState* sampler_diffuse;
	ID3D11Buffer* vs_buffer;
	ID3D11Buffer* ps_globals;
	vector<Shader> shaders;
	Matrix mat_view_proj;
	Vec2 fog_range;
	Color fog_color;
};
