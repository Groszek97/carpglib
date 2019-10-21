#include "EnginePch.h"
#include "EngineCore.h"
#include "Render.h"
#include "Engine.h"
#include "File.h"
#include "App.h"
#include "DirectX.h"
#include <d3dcompiler.h>

Render* app::render;
const DXGI_FORMAT DISPLAY_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;

//=================================================================================================
Render::Render() : initialized(false), factory(nullptr), adapter(nullptr), swap_chain(nullptr), device(nullptr), device_context(nullptr),
render_target(nullptr), depth_stencil_view(nullptr), vsync(true), shaders_dir("shaders"), blend_state(), depth_state()
{
}

//=================================================================================================
Render::~Render()
{
	for(int i = 0; i < 2; ++i)
		SafeRelease(blend_state[i]);
	for(int i = 0; i < DEPTH_MAX; ++i)
		SafeRelease(depth_state[i]);
	SafeRelease(depth_stencil_view);
	SafeRelease(render_target);
	SafeRelease(swap_chain);
	SafeRelease(device_context);

	if(device)
	{
		// write to output directx leaks
#ifdef _DEBUG
		ID3D11Debug* debug;
		device->QueryInterface(__uuidof(ID3D11Debug), (void**)&debug);
		if(debug)
		{
			debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
			debug->Release();
		}
#endif

		device->Release();
	}

	SafeRelease(adapter);
	SafeRelease(factory);
}

//=================================================================================================
void Render::Init()
{
	wnd_size = app::engine->GetWindowSize();

	CreateAdapter();
	CreateDeviceAndSwapChain();
	CreateSizeDependentResources();
	CreateBlendStates();
	CreateDepthStates();
	device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for(int i = 2; i <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; i *= 2)
	{
		uint count;
		if(FAILED(device->CheckMultisampleQualityLevels(DISPLAY_FORMAT, i, &count)) || count == 0)
			break;
		Info("Multisampling x%i (quality levels %u).", i, count);
	}

	initialized = true;
	Info("Render: Directx device created.");
}

//=================================================================================================
void Render::CreateAdapter()
{
	V(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory));

	V(factory->EnumAdapters(0u, &adapter));

	DXGI_ADAPTER_DESC desc;
	V(adapter->GetDesc(&desc));
	Info("Used adapter: %s", ToString(desc.Description));
}

//=================================================================================================
void Render::CreateDeviceAndSwapChain()
{
	DXGI_SWAP_CHAIN_DESC swap_desc = {};
	swap_desc.BufferCount = 1;
	swap_desc.BufferDesc.Width = wnd_size.x;
	swap_desc.BufferDesc.Height = wnd_size.y;
	swap_desc.BufferDesc.Format = DISPLAY_FORMAT;
	swap_desc.BufferDesc.RefreshRate.Numerator = 0;
	swap_desc.BufferDesc.RefreshRate.Denominator = 1;
	swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_desc.OutputWindow = app::engine->GetWindowHandle();
	swap_desc.SampleDesc.Count = 1;
	swap_desc.SampleDesc.Quality = 0;
	swap_desc.Windowed = true;
	swap_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swap_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	int flags = 0;
#ifdef _DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0 };
	D3D_FEATURE_LEVEL feature_level;
	HRESULT result = D3D11CreateDeviceAndSwapChain(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, feature_levels, countof(feature_levels),
		D3D11_SDK_VERSION, &swap_desc, &swap_chain, &device, &feature_level, &device_context);
	if(FAILED(result))
		throw Format("Failed to create device and swap chain (%u).", result);

	// disable alt+enter
	V(factory->MakeWindowAssociation(swap_desc.OutputWindow, DXGI_MWA_NO_WINDOW_CHANGES));
}

//=================================================================================================
void Render::CreateSizeDependentResources()
{
	CreateRenderTarget();
	CreateDepthStencilView();
	device_context->OMSetRenderTargets(1, &render_target, depth_stencil_view);
	SetViewport();
}

//=================================================================================================
void Render::CreateRenderTarget()
{
	HRESULT result;
	ID3D11Texture2D* back_buffer;
	result = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer);
	if(FAILED(result))
		throw Format("Failed to get back buffer (%u).", result);

	// Create the render target view with the back buffer pointer.
	result = device->CreateRenderTargetView(back_buffer, NULL, &render_target);
	if(FAILED(result))
		throw Format("Failed to create render target view (%u).", result);

	// Release pointer to the back buffer as we no longer need it.
	back_buffer->Release();
}

//=================================================================================================
void Render::CreateDepthStencilView()
{
	// create depth buffer texture
	D3D11_TEXTURE2D_DESC tex_desc = {};

	tex_desc.Width = wnd_size.x;
	tex_desc.Height = wnd_size.y;
	tex_desc.MipLevels = 1;
	tex_desc.ArraySize = 1;
	tex_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	tex_desc.CPUAccessFlags = 0;
	tex_desc.MiscFlags = 0;

	ID3D11Texture2D* depth_tex;
	V(device->CreateTexture2D(&tex_desc, nullptr, &depth_tex));

	//==================================================================
	// create depth stencil view from texture
	D3D11_DEPTH_STENCIL_VIEW_DESC view_desc = {};

	view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	view_desc.Texture2D.MipSlice = 0;

	V(device->CreateDepthStencilView(depth_tex, &view_desc, &depth_stencil_view));

	depth_tex->Release();
}

//=================================================================================================
void Render::SetViewport()
{
	D3D11_VIEWPORT viewport;
	viewport.Width = (float)wnd_size.x;
	viewport.Height = (float)wnd_size.y;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	device_context->RSSetViewports(1, &viewport);
}

//=================================================================================================
void Render::CreateBlendStates()
{
	// get disabled blend state
	device_context->OMGetBlendState(&blend_state[0], nullptr, nullptr);

	// create enabled blend state
	D3D11_BLEND_DESC b_desc = { 0 };
	b_desc.RenderTarget[0].BlendEnable = true;
	b_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	b_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	b_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	b_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	b_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	b_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	b_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	HRESULT result = device->CreateBlendState(&b_desc, &blend_state[1]);
	if(FAILED(result))
		throw Format("Failed to create blend state (%u).", result);

	alphablend = false;
}

//=================================================================================================
void Render::CreateDepthStates()
{
	// create depth stencil state
	D3D11_DEPTH_STENCIL_DESC desc = {};
	desc.DepthEnable = true;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	desc.StencilEnable = false;
	desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

	desc.BackFace = desc.FrontFace;

	V(device->CreateDepthStencilState(&desc, &depth_state[DEPTH_YES]));
	device_context->OMSetDepthStencilState(depth_state[DEPTH_YES], 1);
	current_depth_state = DEPTH_YES;

	//==================================================================
	// create depth stencil state with disabled depth test
	desc.DepthEnable = false;
	desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	V(device->CreateDepthStencilState(&desc, &depth_state[DEPTH_NO]));

	//==================================================================
	// create readonly depth stencil state
	desc.DepthEnable = true;
	desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	V(device->CreateDepthStencilState(&desc, &depth_state[DEPTH_READONLY]));
}

//=================================================================================================
void Render::Clear(const Vec4& color)
{
	device_context->ClearRenderTargetView(render_target, (const float*)color);
	device_context->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH, 1.f, 0);
}

//=================================================================================================
void Render::Present()
{
	V(swap_chain->Present(vsync ? 1 : 0, 0));
}

//=================================================================================================
void Render::OnChangeResolution()
{
	const Int2& wnd_size = app::engine->GetWindowSize();

	if(this->wnd_size == wnd_size || !swap_chain)
		return;

	this->wnd_size = wnd_size;

	render_target->Release();
	depth_stencil_view->Release();

	V(swap_chain->ResizeBuffers(1, wnd_size.x, wnd_size.y, DISPLAY_FORMAT, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	CreateSizeDependentResources();
}

//=================================================================================================
TEX Render::CreateTexture(const Int2& size, Color* fill)
{
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = size.x;
	desc.Height = size.y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

	ID3D11Texture2D* tex;
	if(fill)
	{
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = fill;
		initData.SysMemPitch = sizeof(Color) * size.x;
		V(device->CreateTexture2D(&desc, &initData, &tex));
	}
	else
	{
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		V(device->CreateTexture2D(&desc, nullptr, &tex));
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC v_desc = {};
	v_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	v_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	v_desc.Texture2D.MipLevels = 1;

	ID3D11ShaderResourceView* view;
	V(device->CreateShaderResourceView(tex, &v_desc, &view));
	tex->Release();

	return view;
}

//=================================================================================================
void Render::SetAlphaBlend(bool use_alphablend)
{
	if(use_alphablend != alphablend)
	{
		alphablend = use_alphablend;
		device_context->OMSetBlendState(blend_state[alphablend ? 1 : 0], nullptr, 0xFFFFFFFF);
	}
}

//=================================================================================================
void Render::SetDepthState(DepthState state)
{
	assert(state >= 0 && state < DEPTH_MAX);
	if(state == current_depth_state)
		return;
	current_depth_state = state;
	device_context->OMSetDepthStencilState(depth_state[state], 0);
}

//=================================================================================================
void Render::CreateShader(cstring filename, D3D11_INPUT_ELEMENT_DESC* input, uint input_count, ID3D11VertexShader*& vertex_shader,
	ID3D11PixelShader*& pixel_shader, ID3D11InputLayout*& layout, D3D_SHADER_MACRO* macro)
{
	try
	{
		CPtr<ID3DBlob> vs_buf = CompileShader(filename, "vs_main", true, macro);
		HRESULT result = device->CreateVertexShader(vs_buf->GetBufferPointer(), vs_buf->GetBufferSize(), nullptr, &vertex_shader);
		if(FAILED(result))
			throw Format("Failed to create vertex shader (%u).", result);

		CPtr<ID3DBlob> ps_buf = CompileShader(filename, "ps_main", false, macro);
		result = device->CreatePixelShader(ps_buf->GetBufferPointer(), ps_buf->GetBufferSize(), nullptr, &pixel_shader);
		if(FAILED(result))
			throw Format("Failed to create pixel shader (%u).", result);

		result = device->CreateInputLayout(input, input_count, vs_buf->GetBufferPointer(), vs_buf->GetBufferSize(), &layout);
		if(FAILED(result))
			throw Format("Failed to create input layout (%u).", result);
	}
	catch(cstring err)
	{
		throw Format("Failed to create shader '%s': %s", filename, err);
	}
}

//=================================================================================================
ID3DBlob* Render::CompileShader(cstring filename, cstring entry, bool is_vertex, D3D_SHADER_MACRO* macro)
{
	assert(filename && entry);

	cstring target = is_vertex ? "vs_5_0" : "ps_5_0";

	uint flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	flags |= D3DCOMPILE_DEBUG;
#endif

	cstring path = Format("%s/%s", shaders_dir.c_str(), filename);
	ID3DBlob* shader_blob = nullptr;
	ID3DBlob* error_blob = nullptr;
	HRESULT result = D3DCompileFromFile(ToWString(path), macro, nullptr, entry, target, flags, 0, &shader_blob, &error_blob);
	if(FAILED(result))
	{
		SafeRelease(shader_blob);
		if(error_blob)
		{
			cstring err = (cstring)error_blob->GetBufferPointer();
			cstring msg = Format("Failed to compile '%s' function '%s': %s (code %u).", path, entry, err, result);
			error_blob->Release();
			throw msg;
		}
		else
			throw Format("Failed to compile '%s' function '%s' (code %u).", path, entry, result);
	}

	if(error_blob)
	{
		cstring err = (cstring)error_blob->GetBufferPointer();
		Warn("Shader '%s' warnings: %s", path, err);
		error_blob->Release();
	}

	return shader_blob;
}

//=================================================================================================
ID3D11Buffer* Render::CreateConstantBuffer(uint size)
{
	if(size % 16 != 0)
		size = (size / 16 + 1) * 16;

	D3D11_BUFFER_DESC cb_desc;
	cb_desc.Usage = D3D11_USAGE_DYNAMIC;
	cb_desc.ByteWidth = size;
	cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cb_desc.MiscFlags = 0;
	cb_desc.StructureByteStride = 0;

	ID3D11Buffer* buffer;
	HRESULT result = device->CreateBuffer(&cb_desc, NULL, &buffer);
	if(FAILED(result))
		throw Format("Failed to create constant buffer (size:%u; code:%u).", size, result);

	return buffer;
}

//=================================================================================================
ID3D11SamplerState* Render::CreateSampler(TextureAddressMode mode)
{
	D3D11_SAMPLER_DESC sampler_desc;
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_desc.AddressU = (D3D11_TEXTURE_ADDRESS_MODE)mode;
	sampler_desc.AddressV = (D3D11_TEXTURE_ADDRESS_MODE)mode;
	sampler_desc.AddressW = (D3D11_TEXTURE_ADDRESS_MODE)mode;
	sampler_desc.MipLODBias = 0.0f;
	sampler_desc.MaxAnisotropy = 1;
	sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sampler_desc.BorderColor[0] = 0;
	sampler_desc.BorderColor[1] = 0;
	sampler_desc.BorderColor[2] = 0;
	sampler_desc.BorderColor[3] = 0;
	sampler_desc.MinLOD = 0;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

	ID3D11SamplerState* sampler;
	HRESULT result = device->CreateSamplerState(&sampler_desc, &sampler);
	if(FAILED(result))
		throw Format("Failed to create sampler state (%u).", result);

	return sampler;
}
