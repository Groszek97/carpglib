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
	SuperShader(Render* render);
	~SuperShader();
	void OnInit() override;
	void OnReset() override;
	void OnReload() override;
	void OnRelease() override;
	uint GetShaderId(bool animated, bool have_binormals, bool fog, bool specular, bool normal, bool point_light, bool dir_light) const;
	ID3DXEffect* GetShader(uint id);
	ID3DXEffect* CompileShader(uint id);
	ID3DXEffect* GetEffect() const { return shaders.front().e; }
	// new
	void SetFog(Color color, const Vec2& range);
	void SetFogDisabled() { use_fog = false; }
	void SetDirLight(Color ambient_color, Color light_color, const Vec3& light_dir);
	void SetPointLight(Color ambient_color);
	void SetLightingDisabled();

	D3DXHANDLE hMatCombined, hMatWorld, hMatBones, hTint, hAmbientColor, hFogColor, hFogParams, hLightDir, hLightColor, hLights, hSpecularColor,
		hSpecularIntensity, hSpecularHardness, hCameraPos, hTexDiffuse, hTexNormal, hTexSpecular;

private:
	Render* render;
	string code;
	FileTime edit_time;
	ID3DXEffectPool* pool;
	vector<Shader> shaders;
	D3DXHANDLE h_fog_color, h_fog_params, h_ambient_color, h_light_color, h_light_dir;
	// new
	ID3DXEffect* current_effect;
	bool use_fog, use_point_light, use_dir_light;
};
