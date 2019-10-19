#include "EnginePch.h"
#include "EngineCore.h"
#include "GuiShader.h"
#include "Render.h"
#include "Engine.h"
#include "DirectX.h"

//=================================================================================================
GuiShader::GuiShader() : vertex_shader(nullptr), pixel_shader(nullptr), layout(nullptr), vs_buffer(nullptr), ps_buffer(nullptr),
sampler(nullptr), empty_tex(nullptr)
{
	try
	{
		Init();
	}
	catch(cstring err)
	{
		throw Format("Failed to initialize gui shader: %s", err);
	}
}

//=================================================================================================
GuiShader::~GuiShader()
{
	SafeRelease(vertex_shader);
	SafeRelease(pixel_shader);
	SafeRelease(layout);
	SafeRelease(vs_buffer);
	SafeRelease(ps_buffer);
	SafeRelease(sampler);
	SafeRelease(vb);
	SafeRelease(empty_tex);
}

//=================================================================================================
void GuiShader::Init()
{
	device_context = app::render->GetDeviceContext();

	D3D11_INPUT_ELEMENT_DESC desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	app::render->CreateShader("gui.hlsl", desc, countof(desc), vertex_shader, pixel_shader, layout);
	vs_buffer = app::render->CreateConstantBuffer(sizeof(Vec2));
	ps_buffer = app::render->CreateConstantBuffer(sizeof(bool));
	sampler = app::render->CreateSampler(TEX_ADR_CLAMP);

	D3D11_BUFFER_DESC v_desc;
	v_desc.Usage = D3D11_USAGE_DYNAMIC;
	v_desc.ByteWidth = 6 * 256 * sizeof(VParticle);
	v_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	v_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	v_desc.MiscFlags = 0;
	v_desc.StructureByteStride = 0;
	app::render->GetDevice()->CreateBuffer(&v_desc, nullptr, &vb);

	empty_tex = app::render->CreateTexture(Int2(1, 1), &Color::White);
}

//=================================================================================================
void GuiShader::Prepare()
{
	current_tex = nullptr;

	app::render->SetAlphaBlend(true);
	app::render->SetDepthState(Render::DEPTH_NO);

	device_context->IASetInputLayout(layout);
	device_context->VSSetShader(vertex_shader, nullptr, 0);
	device_context->PSSetShader(pixel_shader, nullptr, 0);
	device_context->PSSetSamplers(0, 1, &sampler);
	uint stride = sizeof(VParticle),
		offset = 0;
	device_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

	// vertex shader constants
	const Int2& wnd_size = app::engine->GetWindowSize();
	D3D11_MAPPED_SUBRESOURCE resource;
	V(device_context->Map(vs_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	*(Vec2*)resource.pData = Vec2(wnd_size);
	device_context->Unmap(vs_buffer, 0);
	device_context->VSSetConstantBuffers(0, 1, &vs_buffer);

	// pixel shader constants
	V(device_context->Map(ps_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	*(bool*)resource.pData = false;
	device_context->Unmap(ps_buffer, 0);
	device_context->PSSetConstantBuffers(0, 1, &ps_buffer);
}

//=================================================================================================
void GuiShader::Draw(TEX tex, VParticle* verts, uint quads)
{
	assert(quads > 0);

	// set texture
	if(!tex)
		tex = empty_tex;
	if(tex != current_tex)
	{
		device_context->PSSetShaderResources(0, 1, &tex);
		current_tex = tex;
	}

	// copy to vb
	D3D11_MAPPED_SUBRESOURCE resource;
	V(device_context->Map(vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	memcpy(resource.pData, verts, sizeof(VParticle) * 6 * quads);
	device_context->Unmap(vb, 0);

	// draw
	device_context->Draw(quads * 6, 0);
}
