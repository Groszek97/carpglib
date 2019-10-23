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
		SPECULAR_MAP,
		NORMAL_MAP,
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
		Vec3 camera_pos;
	};

	struct VertexLocals
	{
		Matrix mat_combined;
		Matrix mat_world;
		Matrix mat_bones[32];
	};

	struct PixelGlobals
	{
		Vec4 ambient_color;
		Vec4 light_color;
		Vec3 light_dir;
		float _pad;
		Vec4 fog_color;
		Vec4 fog_params;
	};

	struct Light
	{
		Vec4 color;
		Vec3 pos;
		float range;
	};

	struct PixelLocals
	{
		Vec4 tint;
		Light lights[3];
	};

	struct PixelMaterial
	{
		Vec3 specular_color;
		float specular_hardness;
		float specular_intensity;
	};

public:
	SuperShader();
	~SuperShader();
	uint GetShaderId(bool have_weight, bool have_binormals, bool animated, bool fog, bool specular, bool normal, bool point_light, bool dir_light) const;
	void SetShader(uint id);
	void SetAmbientColor(Color color) { ambient_color = color; }
	void SetDirectionLight(Color color, const Vec3& dir)
	{
		light_color = color;
		light_dir = dir;
	}
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
	ID3D11SamplerState* sampler_normal;
	ID3D11SamplerState* sampler_specular;
	ID3D11Buffer* vs_globals;
	ID3D11Buffer* vs_locals;
	ID3D11Buffer* ps_globals;
	ID3D11Buffer* ps_locals;
	ID3D11Buffer* ps_material;
	TEX tex_normal, tex_specular;
	vector<Shader> shaders;
	Matrix mat_view_proj;
	Vec3 light_dir;
	Vec2 fog_range;
	Color fog_color, ambient_color, light_color;
	bool set_lights, set_normal_map, set_specular_map;
};
