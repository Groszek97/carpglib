#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
class SuperShader : public ShaderHandler
{
	enum Switches
	{
		ANIMATED,
		HAVE_BINORMALS,
		FOG,
		SPECULAR,
		NORMAL,
		POINT_LIGHT,
		DIR_LIGHT
	};

	struct Shader
	{
		ID3DXEffect* e;
		uint id;
	};

public:
	SuperShader();
	~SuperShader();
	void OnInit() override;
	void OnReset() override;
	void OnReload() override;
	void OnRelease() override;
	uint GetShaderId(bool animated, bool have_binormals, bool fog, bool specular, bool normal, bool point_light, bool dir_light) const;
	ID3DXEffect* GetShader(uint id);
	ID3DXEffect* CompileShader(uint id);
	ID3DXEffect* GetEffect() const { return shaders.front().e; }

	D3DXHANDLE h_mat_combined, h_mat_world, h_mat_bones, h_tint, h_ambient_color, h_fog_color, h_fog_params, h_light_dir, h_light_color, h_lights,
		h_specular_color, h_specular_intensity, h_specular_hardness, h_camera_pos, h_tex_diffuse, h_tex_normal, h_tex_specular;

private:
	string code;
	FileTime edit_time;
	ID3DXEffectPool* pool;
	vector<Shader> shaders;
};
