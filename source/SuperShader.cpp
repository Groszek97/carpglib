#include "EnginePch.h"
#include "EngineCore.h"
#include "SuperShader.h"
#include "SceneNode.h"
#include "Camera.h"
#include "Render.h"
#include "MeshInstance.h"
#include "DirectX.h"
#include <d3dcompiler.h>

//=================================================================================================
SuperShader::SuperShader() : sampler_diffuse(nullptr), vs_globals(nullptr), vs_locals(nullptr), ps_globals(nullptr), ps_locals(nullptr)
{
	try
	{
		Init();
	}
	catch(cstring err)
	{
		throw Format("Failed to initialize super shader: %s", err);
	}
}

//=================================================================================================
SuperShader::~SuperShader()
{
	for(Shader& shader : shaders)
	{
		SafeRelease(shader.vertex_shader);
		SafeRelease(shader.pixel_shader);
		SafeRelease(shader.layout);
	}
	SafeRelease(sampler_diffuse);
	SafeRelease(vs_globals);
	SafeRelease(vs_locals);
	SafeRelease(ps_globals);
	SafeRelease(ps_locals);
}

//=================================================================================================
void SuperShader::Init()
{
	device_context = app::render->GetDeviceContext();

	sampler_diffuse = app::render->CreateSampler();

	vs_globals = app::render->CreateConstantBuffer(sizeof(VertexGlobals));
	vs_locals = app::render->CreateConstantBuffer(sizeof(VertexLocals));
	ps_globals = app::render->CreateConstantBuffer(sizeof(PixelGlobals));
	ps_locals = app::render->CreateConstantBuffer(sizeof(PixelLocals));
}

//=================================================================================================
uint SuperShader::GetShaderId(bool have_weight, bool have_binormals, bool animated, bool fog, bool specular, bool normal, bool point_light, bool dir_light) const
{
	uint id = 0;
	if(have_weight)
		id |= (1 << HAVE_WEIGHT);
	if(have_binormals)
		id |= (1 << HAVE_BINORMALS);
	if(animated)
		id |= (1 << ANIMATED);
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
void SuperShader::Prepare(Camera& camera)
{
	mat_view_proj = camera.GetViewProjMatrix();

	app::render->SetAlphaBlend(false);
	app::render->SetDepthState(Render::DEPTH_YES);

	// set vs globals
	D3D11_MAPPED_SUBRESOURCE resource;
	V(device_context->Map(vs_globals, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	VertexGlobals& vs = *(VertexGlobals*)resource.pData;
	vs.camera_pos = camera.from;
	device_context->Unmap(vs_globals, 0);

	// set ps globals
	V(device_context->Map(ps_globals, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	PixelGlobals& ps = *(PixelGlobals*)resource.pData;
	ps.ambient_color = (Vec4)ambient_color;
	ps.light_color = (Vec4)light_color;
	ps.light_dir = light_dir;
	ps.fog_color = (Vec4)fog_color;
	ps.fog_params = Vec4(fog_range.x, fog_range.y, fog_range.y - fog_range.x, 0);
	device_context->Unmap(ps_globals, 0);

	device_context->PSSetSamplers(0, 1, &sampler_diffuse);
	ID3D11Buffer* vs_constants[] = { vs_globals, vs_locals };
	device_context->VSSetConstantBuffers(0, 2, vs_constants);
	ID3D11Buffer* ps_constants[] = { ps_globals, ps_locals };
	device_context->PSSetConstantBuffers(0, 2, ps_constants);
}

//=================================================================================================
void SuperShader::SetShader(uint id)
{
	Shader& shader = GetShader(id);

	device_context->IASetInputLayout(shader.layout);
	device_context->VSSetShader(shader.vertex_shader, nullptr, 0);
	device_context->PSSetShader(shader.pixel_shader, nullptr, 0);
}

//=================================================================================================
SuperShader::Shader& SuperShader::GetShader(uint id)
{
	for(Shader& shader : shaders)
	{
		if(shader.id == id)
			return shader;
	}

	return CompileShader(id);
}

//=================================================================================================
SuperShader::Shader& SuperShader::CompileShader(uint id)
{
	Info("Compiling super shader %u.", id);

	// setup layout
	vector<D3D11_INPUT_ELEMENT_DESC> layout_desc;
	layout_desc.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 });
	if(IsSet(id, 1 << HAVE_WEIGHT))
	{
		layout_desc.push_back({ "BLENDWEIGHT", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 });
		layout_desc.push_back({ "BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 });
	}
	layout_desc.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 });
	layout_desc.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 });

	// setup macros
	D3D_SHADER_MACRO macros[8] = {};
	uint i = 0;

	if(IsSet(id, 1 << HAVE_WEIGHT))
	{
		macros[i].Name = "HAVE_WEIGHT";
		macros[i].Definition = "1";
		++i;
	}
	if(IsSet(id, 1 << HAVE_BINORMALS))
	{
		macros[i].Name = "HAVE_BINORMALS";
		macros[i].Definition = "1";
		++i;
	}
	if(IsSet(id, 1 << ANIMATED))
	{
		macros[i].Name = "ANIMATED";
		macros[i].Definition = "1";
		++i;
	}
	if(IsSet(id, 1 << FOG))
	{
		macros[i].Name = "FOG";
		macros[i].Definition = "1";
		++i;
	}
	if(IsSet(id, 1 << SPECULAR))
	{
		macros[i].Name = "SPECULAR_MAP";
		macros[i].Definition = "1";
		++i;
	}
	if(IsSet(id, 1 << NORMAL))
	{
		macros[i].Name = "NORMAL_MAP";
		macros[i].Definition = "1";
		++i;
	}
	if(IsSet(id, 1 << POINT_LIGHT))
	{
		macros[i].Name = "POINT_LIGHT";
		macros[i].Definition = "1";
		++i;
	}
	else if(IsSet(id, 1 << DIR_LIGHT))
	{
		macros[i].Name = "DIR_LIGHT";
		macros[i].Definition = "1";
		++i;
	}

	// compile
	Shader shader;
	shader.id = id;
	app::render->CreateShader("super.hlsl", layout_desc.data(), layout_desc.size(), shader.vertex_shader, shader.pixel_shader, shader.layout, macros);
	shaders.push_back(shader);
	return shaders.back();
}

//=================================================================================================
void SuperShader::Draw(SceneNode* node)
{
	assert(node);

	Matrix mat_world = Matrix::Rotation(node->rot) * Matrix::Translation(node->pos);
	Matrix mat_combined = mat_world * mat_view_proj;

	// set vs locals
	D3D11_MAPPED_SUBRESOURCE resource;
	V(device_context->Map(vs_locals, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	VertexLocals& vs = *(VertexLocals*)resource.pData;
	vs.mat_combined = mat_combined.Transpose();
	vs.mat_world = mat_world.Transpose();
	if(node->mesh_inst)
	{
		node->mesh_inst->SetupBones();
		for(int i = 0; i < node->mesh->head.n_bones; ++i)
			vs.mat_bones[i] = node->mesh_inst->mat_bones[i].Transpose();
	}
	device_context->Unmap(vs_locals, 0);

	// set buffers
	Mesh* mesh = node->mesh;
	uint stride = mesh->vertex_size,
		offset = 0;
	device_context->IASetVertexBuffers(0, 1, &mesh->vb, &stride, &offset);
	device_context->IASetIndexBuffer(mesh->ib, DXGI_FORMAT_R16_UINT, 0);

	// draw
	for(Mesh::Submesh& sub : mesh->subs)
	{
		// set ps locals
		V(device_context->Map(ps_locals, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
		PixelLocals& ps = *(PixelLocals*)resource.pData;
		ps.specular_color = sub.specular_color;
		ps.specular_hardness = (float)sub.specular_hardness;
		ps.specular_intensity = sub.specular_intensity;
		device_context->Unmap(ps_locals, 0);

		device_context->PSSetShaderResources(0, 1, &sub.tex->tex);
		device_context->DrawIndexed(sub.tris * 3, sub.first * 3, sub.min_ind);
	}
}
