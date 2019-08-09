#include "EnginePch.h"
#include "EngineCore.h"
#include "SuperShader.h"
#include "File.h"
#include "Render.h"
#include "SceneNode.h"
#include "DirectX.h"

//=================================================================================================
SuperShader::SuperShader() : render(app::render), pool(nullptr)
{
}

//=================================================================================================
SuperShader::~SuperShader()
{
	SafeRelease(pool);
}

//=================================================================================================
void SuperShader::OnInit()
{
	if(!pool)
		V(D3DXCreateEffectPool(&pool));

	cstring path = Format("%s/super.fx", render->GetShadersDir().c_str());
	FileReader f(path);
	if(!f)
		throw Format("Failed to open file '%s'.", path);
	FileTime file_time = f.GetTime();
	if(file_time != edit_time)
	{
		f.ReadToString(code);
		edit_time = file_time;
	}

	Info("Setting up super shader parameters.");
	GetShader(0);
	ID3DXEffect* e = shaders[0].e;
	hMatCombined = e->GetParameterByName(nullptr, "matCombined");
	hMatWorld = e->GetParameterByName(nullptr, "matWorld");
	hMatBones = e->GetParameterByName(nullptr, "matBones");
	hTint = e->GetParameterByName(nullptr, "tint");
	hAmbientColor = e->GetParameterByName(nullptr, "ambientColor");
	hFogColor = e->GetParameterByName(nullptr, "fogColor");
	hFogParams = e->GetParameterByName(nullptr, "fogParams");
	hLightDir = e->GetParameterByName(nullptr, "lightDir");
	hLightColor = e->GetParameterByName(nullptr, "lightColor");
	hLights = e->GetParameterByName(nullptr, "lights");
	hSpecularColor = e->GetParameterByName(nullptr, "specularColor");
	hSpecularIntensity = e->GetParameterByName(nullptr, "specularIntensity");
	hSpecularHardness = e->GetParameterByName(nullptr, "specularHardness");
	hCameraPos = e->GetParameterByName(nullptr, "cameraPos");
	hTexDiffuse = e->GetParameterByName(nullptr, "texDiffuse");
	hTexNormal = e->GetParameterByName(nullptr, "texNormal");
	hTexSpecular = e->GetParameterByName(nullptr, "texSpecular");
	assert(hMatCombined && hMatWorld && hMatBones && hTint && hAmbientColor && hFogColor && hFogParams && hLightDir && hLightColor && hLights && hSpecularColor
		&& hSpecularIntensity && hSpecularHardness && hCameraPos && hTexDiffuse && hTexNormal && hTexSpecular);
}

//=================================================================================================
void SuperShader::OnReload()
{
	for(vector<Shader>::iterator it = shaders.begin(), end = shaders.end(); it != end; ++it)
		V(it->e->OnResetDevice());
}

//=================================================================================================
void SuperShader::OnReset()
{
	for(vector<Shader>::iterator it = shaders.begin(), end = shaders.end(); it != end; ++it)
		V(it->e->OnLostDevice());
}

//=================================================================================================
void SuperShader::OnRelease()
{
	for(vector<Shader>::iterator it = shaders.begin(), end = shaders.end(); it != end; ++it)
		SafeRelease(it->e);
	shaders.clear();
}

//=================================================================================================
uint SuperShader::GetShaderId(bool animated, bool have_binormals, bool fog, bool specular, bool normal, bool point_light, bool dir_light) const
{
	uint id = 0;
	if(animated)
		id |= (1 << ANIMATED);
	if(have_binormals)
		id |= (1 << HAVE_BINORMALS);
	if(fog)
		id |= (1 << FOG);
	if(specular)
		id |= (1 << SPECULAR);
	if(normal)
		id |= (1 << NORMAL);
	if(point_light)
		id |= (1 << POINT_LIGHT);
	if(dir_light)
		id |= (1 << DIR_LIGHT);
	return id;
}

//=================================================================================================
ID3DXEffect* SuperShader::GetShader(uint id)
{
	for(vector<Shader>::iterator it = shaders.begin(), end = shaders.end(); it != end; ++it)
	{
		if(it->id == id)
			return it->e;
	}

	return CompileShader(id);
}

//=================================================================================================
ID3DXEffect* SuperShader::CompileShader(uint id)
{
	int shader_version = render->GetShaderVersion();
	D3DXMACRO macros[10] = { 0 };
	uint i = 0;

	if(IS_SET(id, 1 << ANIMATED))
	{
		macros[i].Name = "ANIMATED";
		macros[i].Definition = "1";
		++i;
	}
	if(IS_SET(id, 1 << HAVE_BINORMALS))
	{
		macros[i].Name = "HAVE_BINORMALS";
		macros[i].Definition = "1";
		++i;
	}
	if(IS_SET(id, 1 << FOG))
	{
		macros[i].Name = "FOG";
		macros[i].Definition = "1";
		++i;
	}
	if(IS_SET(id, 1 << SPECULAR))
	{
		macros[i].Name = "SPECULAR_MAP";
		macros[i].Definition = "1";
		++i;
	}
	if(IS_SET(id, 1 << NORMAL))
	{
		macros[i].Name = "NORMAL_MAP";
		macros[i].Definition = "1";
		++i;
	}
	if(IS_SET(id, 1 << POINT_LIGHT))
	{
		macros[i].Name = "POINT_LIGHT";
		macros[i].Definition = "1";
		++i;

		macros[i].Name = "LIGHTS";
		macros[i].Definition = (shader_version == 2 ? "2" : "3");
		++i;
	}
	else if(IS_SET(id, 1 << DIR_LIGHT))
	{
		macros[i].Name = "DIR_LIGHT";
		macros[i].Definition = "1";
		++i;
	}

	macros[i].Name = "VS_VERSION";
	macros[i].Definition = (shader_version == 3 ? "vs_3_0" : "vs_2_0");
	++i;

	macros[i].Name = "PS_VERSION";
	macros[i].Definition = (shader_version == 3 ? "ps_3_0" : "ps_2_0");
	++i;

	Info("Compiling super shader: %u", id);

	CompileShaderParams params = { "super.fx" };
	params.cache_name = Format("%d_super%u.fcx", shader_version, id);
	params.file_time = edit_time;
	params.input = &code;
	params.macros = macros;
	params.pool = pool;

	Shader& s = Add1(shaders);
	s.e = render->CompileShader(params);
	s.id = id;

	return s.e;
}

//void SuperShader::Prepare()
//{
//	effect = shaders.front().e;
//	current_id = 0xFFFFFFFF;
//	use_fog = false;
//
//	D3DXHANDLE tech;
//	V(effect->FindNextValidTechnique(nullptr, &tech));
//	V(effect->SetTechnique(tech));
//}
//
//void SuperShader::SetFog(const Vec2& range, Color color)
//{
//	assert(range.x >= 0.f && range.x <= range.y);
//	V(effect->SetVector(hFogParams, reinterpret_cast<D3DXVECTOR4*>(&Vec4(range.x, range.y, range.y - range.x, 0.f))));
//	Vec4 c = color;
//	V(effect->SetVector(hFogColor, reinterpret_cast<D3DXVECTOR4*>(&c)));
//}
//
//void SuperShader::Begin()
//{
//	uint passes;
//	V(effect->Begin(&passes, 0));
//	V(effect->BeginPass(0));
//}
//
//void SuperShader::Draw(SceneNode* node)
//{
//	uint id = GetShaderId(node->mesh_inst != nullptr, false, use_fog, false, false, false, false);
//	if(id != current_id)
//	{
//		V(e->EndPass());
//		V(e->End());
//		e = shader->GetShader(id);
//		V(e->FindNextValidTechnique(nullptr, &tech));
//		V(e->SetTechnique(tech));
//		V(e->Begin(&passes, 0));
//		V(e->BeginPass(0));
//		current_id = id;
//	}
//
//	Mesh& mesh = *node->mesh;
//
//	mat_world = node->GetWorldMatrix();
//	mat_combined = mat_world * mat_view_proj;
//
//	V(e->SetMatrix(shader->hMatCombined, (D3DXMATRIX*)&mat_combined));
//	V(e->SetMatrix(shader->hMatWorld, (D3DXMATRIX*)&mat_world));
//	if(node->mesh_inst != nullptr)
//	{
//		node->mesh_inst->SetupBones();
//		V(e->SetMatrixArray(shader->hMatBones, (D3DXMATRIX*)node->mesh_inst->mat_bones.data(), node->mesh_inst->mat_bones.size()));
//	}
//	V(device->SetVertexDeclaration(app::render->GetVertexDeclaration(mesh.vertex_decl)));
//	V(device->SetStreamSource(0, mesh.vb, 0, mesh.vertex_size));
//	V(device->SetIndices(mesh.ib));
//
//	for(int i = 0; i < mesh.head.n_subs; ++i)
//	{
//		const Mesh::Submesh& sub = mesh.subs[i];
//
//		// texture
//		V(e->SetTexture(shader->hTexDiffuse, sub.tex->tex));
//		//if(cl_normalmap && IS_SET(current_flags, SceneNode::F_NORMAL_MAP))
//		//	V(e->SetTexture(shader->hTexNormal, sub.tex_normal->tex));
//		//if(cl_specularmap && IS_SET(current_flags, SceneNode::F_SPECULAR_MAP))
//		//	V(e->SetTexture(shader->hTexSpecular, sub.tex_specular->tex));
//
//		// ustawienia œwiat³a
//		//V(e->SetVector(shader->hSpecularColor, (D3DXVECTOR4*)&sub.specular_color));
//		//V(e->SetFloat(shader->hSpecularIntensity, sub.specular_intensity));
//		//V(e->SetFloat(shader->hSpecularHardness, (float)sub.specular_hardness));
//
//		V(e->CommitChanges());
//		V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sub.min_ind, sub.n_ind, sub.first * 3, sub.tris));
//	}
//}
//
//void SuperShader::End()
//{
//	V(effect->EndPass());
//	V(effect->End());
//}
