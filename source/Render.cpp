#include "EnginePch.h"
#include "EngineCore.h"
#include "Render.h"
//#include "RenderTarget.h"
#include "Engine.h"
#include "ShaderHandler.h"
#include "File.h"
#include "App.h"
#include "DirectX.h"
#include <d3dcompiler.h>
//
#include "ResourceManager.h"

Render* app::render;
const DXGI_FORMAT DISPLAY_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;

//=================================================================================================
Render::Render() : initialized(false), factory(nullptr), adapter(nullptr), swap_chain(nullptr), device(nullptr), device_context(nullptr),
render_target(nullptr), depth_stencil_view(nullptr), vsync(true), shaders_dir("shaders"), multisampling(0), multisampling_quality(0)
{
	//for(int i = 0; i < VDI_MAX; ++i)
	//	vertex_decl[i] = nullptr;
}

//=================================================================================================
Render::~Render()
{
	/*for(ShaderHandler* shader : shaders)
	{
		if(!shader->IsManual())
		{
			shader->OnRelease();
			delete shader;
		}
	}
	for(RenderTarget* target : targets)
	{
		SafeRelease(target->surf);
		delete target;
	}
	for(int i = 0; i < VDI_MAX; ++i)
		SafeRelease(vertex_decl[i]);
	if(device)
	{
		device->SetStreamSource(0, nullptr, 0, 0);
		device->SetIndices(nullptr);
	}*/

	SafeRelease(vertex_shader);
	SafeRelease(pixel_shader);
	SafeRelease(layout);
	SafeRelease(vs_buffer);
	SafeRelease(vb);
	SafeRelease(sampler);

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
	// create direct3d object
	/*d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if(!d3d)
		throw "Render: Failed to create direct3d object.";

	// get adapters count
	uint adapters = d3d->GetAdapterCount();
	Info("Render: Adapters count: %u", adapters);

	// get adapters info
	HRESULT hr;
	D3DADAPTER_IDENTIFIER9 adapter;
	for(uint i = 0; i < adapters; ++i)
	{
		hr = d3d->GetAdapterIdentifier(i, 0, &adapter);
		if(FAILED(hr))
			Warn("Render: Can't get info about adapter %d (%d).", i, hr);
		else
		{
			Info("Render: Adapter %d: %s, version %d.%d.%d.%d", i, adapter.Description, HIWORD(adapter.DriverVersion.HighPart),
				LOWORD(adapter.DriverVersion.HighPart), HIWORD(adapter.DriverVersion.LowPart), LOWORD(adapter.DriverVersion.LowPart));
		}
	}
	if(used_adapter > (int)adapters)
	{
		Warn("Render: Invalid adapter %d, defaulting to 0.", used_adapter);
		used_adapter = 0;
	}

	// check shaders version
	D3DCAPS9 caps;
	d3d->GetDeviceCaps(used_adapter, D3DDEVTYPE_HAL, &caps);
	if(D3DVS_VERSION(2, 0) > caps.VertexShaderVersion || D3DPS_VERSION(2, 0) > caps.PixelShaderVersion)
	{
		throw Format("Render: Too old graphic card! This game require vertex and pixel shader in version 2.0+. "
			"Your card support:\nVertex shader: %d.%d\nPixel shader: %d.%d",
			D3DSHADER_VERSION_MAJOR(caps.VertexShaderVersion), D3DSHADER_VERSION_MINOR(caps.VertexShaderVersion),
			D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion), D3DSHADER_VERSION_MINOR(caps.PixelShaderVersion));
	}
	else
	{
		Info("Supported shader version vertex: %d.%d, pixel: %d.%d.",
			D3DSHADER_VERSION_MAJOR(caps.VertexShaderVersion), D3DSHADER_VERSION_MINOR(caps.VertexShaderVersion),
			D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion), D3DSHADER_VERSION_MINOR(caps.PixelShaderVersion));

		int version = min(D3DSHADER_VERSION_MAJOR(caps.VertexShaderVersion), D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion));
		if(shader_version == -1 || shader_version > version || shader_version < 2)
			shader_version = version;

		Info("Using shader version %d.", shader_version);
	}

	// check texture types
	bool fullscreen = app::engine->IsFullscreen();
	hr = d3d->CheckDeviceType(used_adapter, D3DDEVTYPE_HAL, DISPLAY_FORMAT, BACKBUFFER_FORMAT, fullscreen ? FALSE : TRUE);
	if(FAILED(hr))
		throw Format("Render: Unsupported backbuffer type %s for display %s! (%d)", STRING(BACKBUFFER_FORMAT), STRING(DISPLAY_FORMAT), hr);

	hr = d3d->CheckDeviceFormat(used_adapter, D3DDEVTYPE_HAL, DISPLAY_FORMAT, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, ZBUFFER_FORMAT);
	if(FAILED(hr))
		throw Format("Render: Unsupported depth buffer type %s for display %s! (%d)", STRING(ZBUFFER_FORMAT), STRING(DISPLAY_FORMAT), hr);

	hr = d3d->CheckDepthStencilMatch(used_adapter, D3DDEVTYPE_HAL, DISPLAY_FORMAT, D3DFMT_A8R8G8B8, ZBUFFER_FORMAT);
	if(FAILED(hr))
		throw Format("Render: Unsupported render target D3DFMT_A8R8G8B8 with display %s and depth buffer %s! (%d)",
		STRING(DISPLAY_FORMAT), STRING(BACKBUFFER_FORMAT), hr);

	// check multisampling
	DWORD levels, levels2;
	if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8, fullscreen ? FALSE : TRUE,
		(D3DMULTISAMPLE_TYPE)multisampling, &levels))
		&& SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, D3DFMT_D24S8, fullscreen ? FALSE : TRUE,
		(D3DMULTISAMPLE_TYPE)multisampling, &levels2)))
	{
		levels = min(levels, levels2);
		if(multisampling_quality < 0 || multisampling_quality >= (int)levels)
		{
			Warn("Render: Unavailable multisampling quality, changed to 0.");
			multisampling_quality = 0;
		}
	}
	else
	{
		Warn("Render: Your graphic card don't support multisampling x%d. Maybe it's only available in fullscreen mode. "
			"Multisampling was turned off.", multisampling);
		multisampling = 0;
		multisampling_quality = 0;
	}

	LogMultisampling();
	LogAndSelectResolution();

	// gather params
	D3DPRESENT_PARAMETERS d3dpp = { 0 };
	GatherParams(d3dpp);

	// available modes
	const DWORD mode[] = {
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		D3DCREATE_MIXED_VERTEXPROCESSING,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING
	};
	const cstring mode_str[] = {
		"hardware",
		"mixed",
		"software"
	};

	// try to create device in one of modes
	for(uint i = 0; i < 3; ++i)
	{
		DWORD sel_mode = mode[i];
		hr = d3d->CreateDevice(used_adapter, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, sel_mode, &d3dpp, &device);

		if(SUCCEEDED(hr))
		{
			Info("Render: Created direct3d device in %s mode.", mode_str[i]);
			break;
		}
	}

	// failed to create device
	if(FAILED(hr))
		throw Format("Render: Failed to create direct3d device (%d).", hr);

	// create sprite
	hr = D3DXCreateSprite(device, &sprite);
	if(FAILED(hr))
		throw Format("Render: Failed to create direct3dx sprite (%d).", hr);

	SetDefaultRenderState();
	CreateVertexDeclarations();

	initialized = true;
	Info("Render: Directx device created.");*/

	wnd_size = app::engine->GetWindowSize();

	CreateAdapter();
	CreateDeviceAndSwapChain();
	CreateSizeDependentResources();
	device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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

	D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
	D3D_FEATURE_LEVEL feature_level;
	HRESULT result = D3D11CreateDeviceAndSwapChain(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, feature_levels, countof(feature_levels),
		D3D11_SDK_VERSION, &swap_desc, &swap_chain, &device, &feature_level, &device_context);
	if(FAILED(result))
		throw Format("Failed to create device and swap chain (%u).", result);

	/*Info("Created device with '%s' feature level.", GetFeatureLevelString(feature_level));
	if(feature_level == D3D_FEATURE_LEVEL_11_0)
	{
		vs_target_version = "vs_5_0";
		ps_target_version = "ps_5_0";
	}
	else
	{
		vs_target_version = "vs_4_0";
		ps_target_version = "ps_4_0";
	}*/

	// disable alt+enter
	V(factory->MakeWindowAssociation(swap_desc.OutputWindow, DXGI_MWA_NO_WINDOW_CHANGES));
}


void Render::CreateSizeDependentResources()
{
	CreateRenderTarget();
	CreateDepthStencilView();
	device_context->OMSetRenderTargets(1, &render_target, depth_stencil_view);
	SetViewport();
}

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
void Render::LogMultisampling()
{
	/*LocalString s = "Render: Available multisampling: ";

	for(int j = 2; j <= 16; ++j)
	{
		DWORD levels, levels2;
		if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, BACKBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels))
			&& SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, ZBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels2)))
		{
			s += Format("x%d(%d), ", j, min(levels, levels2));
		}
	}

	if(s.at_back(1) == ':')
		s += "none";
	else
		s.pop(2);

	Info(s);*/
}

//=================================================================================================
void Render::LogAndSelectResolution()
{
	/*struct Res
	{
		int w, h, hz;

		Res(int w, int h, int hz) : w(w), h(h), hz(hz) {}
		bool operator < (const Res& r) const
		{
			if(w > r.w)
				return false;
			else if(w < r.w)
				return true;
			else if(h > r.h)
				return false;
			else if(h < r.h)
				return true;
			else if(hz > r.hz)
				return false;
			else if(hz < r.hz)
				return true;
			else
				return false;
		}
	};

	vector<Res> ress;
	LocalString str = "Render: Available display modes:";
	uint display_modes = d3d->GetAdapterModeCount(used_adapter, DISPLAY_FORMAT);
	Int2 wnd_size = app::engine->GetWindowSize();
	int best_hz = 0, best_valid_hz = 0;
	bool res_valid = false, hz_valid = false;
	for(uint i = 0; i < display_modes; ++i)
	{
		D3DDISPLAYMODE d_mode;
		V(d3d->EnumAdapterModes(used_adapter, DISPLAY_FORMAT, i, &d_mode));
		if(d_mode.Width < (uint)Engine::MIN_WINDOW_SIZE.x || d_mode.Height < (uint)Engine::MIN_WINDOW_SIZE.y)
			continue;
		ress.push_back(Res(d_mode.Width, d_mode.Height, d_mode.RefreshRate));
		if(d_mode.Width == (uint)Engine::DEFAULT_WINDOW_SIZE.x && d_mode.Height == (uint)Engine::DEFAULT_WINDOW_SIZE.y)
		{
			if(d_mode.RefreshRate > (uint)best_hz)
				best_hz = d_mode.RefreshRate;
		}
		if(d_mode.Width == wnd_size.x && d_mode.Height == wnd_size.y)
		{
			res_valid = true;
			if(d_mode.RefreshRate == refresh_hz)
				hz_valid = true;
			if((int)d_mode.RefreshRate > best_valid_hz)
				best_valid_hz = d_mode.RefreshRate;
		}
	}
	std::sort(ress.begin(), ress.end());
	int cw = 0, ch = 0;
	for(vector<Res>::iterator it = ress.begin(), end = ress.end(); it != end; ++it)
	{
		Res& r = *it;
		if(r.w != cw || r.h != ch)
		{
			if(it != ress.begin())
				str += " Hz)";
			str += Format("\n\t%dx%d (%d", r.w, r.h, r.hz);
			cw = r.w;
			ch = r.h;
		}
		else
			str += Format(", %d", r.hz);
	}
	str += " Hz)";
	Info(str->c_str());

	// adjust selected resolution
	if(!res_valid)
	{
		if(wnd_size.x != 0)
		{
			Warn("Render: Resolution %dx%d is not valid, defaulting to %dx%d (%d Hz).", wnd_size.x, wnd_size.y,
				Engine::DEFAULT_WINDOW_SIZE.x, Engine::DEFAULT_WINDOW_SIZE.y, best_hz);
		}
		else
			Info("Render: Defaulting resolution to %dx%dx (%d Hz).", Engine::DEFAULT_WINDOW_SIZE.x, Engine::DEFAULT_WINDOW_SIZE.y, best_hz);
		refresh_hz = best_hz;
		app::engine->SetWindowSizeInternal(Engine::DEFAULT_WINDOW_SIZE);
	}
	else if(!hz_valid)
	{
		if(refresh_hz != 0)
			Warn("Render: Refresh rate %d Hz is not valid, defaulting to %d Hz.", refresh_hz, best_valid_hz);
		else
			Info("Render: Defaulting refresh rate to %d Hz.", best_valid_hz);
		refresh_hz = best_valid_hz;
	}*/
}

//=================================================================================================
void Render::SetDefaultRenderState()
{
	/*V(device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD));
	V(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
	V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));
	V(device->SetRenderState(D3DRS_ALPHAREF, 200));
	V(device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL));*/

	r_alphatest = false;
	r_alphablend = false;
	r_nocull = false;
	r_nozwrite = false;
}

//=================================================================================================
void Render::CreateVertexDeclarations()
{
	/*const D3DVERTEXELEMENT9 Default[] = {
		{0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,		0},
		{0, 12,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_NORMAL,		0},
		{0, 24, D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TEXCOORD,		0},
		D3DDECL_END()
	};
	V(device->CreateVertexDeclaration(Default, &vertex_decl[VDI_DEFAULT]));

	const D3DVERTEXELEMENT9 Animated[] = {
		{0,	0,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,		0},
		{0,	12,	D3DDECLTYPE_FLOAT1,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_BLENDWEIGHT,	0},
		{0,	16,	D3DDECLTYPE_UBYTE4,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_BLENDINDICES,	0},
		{0,	20,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_NORMAL,		0},
		{0,	32,	D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TEXCOORD,		0},
		D3DDECL_END()
	};
	V(device->CreateVertexDeclaration(Animated, &vertex_decl[VDI_ANIMATED]));

	const D3DVERTEXELEMENT9 Tangents[] = {
		{0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,		0},
		{0, 12,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_NORMAL,		0},
		{0, 24, D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TEXCOORD,		0},
		{0,	32,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TANGENT,		0},
		{0,	44,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_BINORMAL,		0},
		D3DDECL_END()
	};
	V(device->CreateVertexDeclaration(Tangents, &vertex_decl[VDI_TANGENT]));

	const D3DVERTEXELEMENT9 AnimatedTangents[] = {
		{0,	0,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,		0},
		{0,	12,	D3DDECLTYPE_FLOAT1,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_BLENDWEIGHT,	0},
		{0,	16,	D3DDECLTYPE_UBYTE4,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_BLENDINDICES,	0},
		{0,	20,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_NORMAL,		0},
		{0,	32,	D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TEXCOORD,		0},
		{0,	40,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TANGENT,		0},
		{0,	52,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_BINORMAL,		0},
		D3DDECL_END()
	};
	V(device->CreateVertexDeclaration(AnimatedTangents, &vertex_decl[VDI_ANIMATED_TANGENT]));

	const D3DVERTEXELEMENT9 Tex[] = {
		{0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,		0},
		{0, 12, D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TEXCOORD,		0},
		D3DDECL_END()
	};
	V(device->CreateVertexDeclaration(Tex, &vertex_decl[VDI_TEX]));

	const D3DVERTEXELEMENT9 Color[] = {
		{0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,		0},
		{0, 12, D3DDECLTYPE_FLOAT4,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_COLOR,			0},
		D3DDECL_END()
	};
	V(device->CreateVertexDeclaration(Color, &vertex_decl[VDI_COLOR]));

	const D3DVERTEXELEMENT9 Particle[] = {
		{0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,		0},
		{0, 12, D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TEXCOORD,		0},
		{0, 20, D3DDECLTYPE_FLOAT4,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_COLOR,			0},
		D3DDECL_END()
	};
	V(device->CreateVertexDeclaration(Particle, &vertex_decl[VDI_PARTICLE]));

	const D3DVERTEXELEMENT9 Pos[] = {
		{0,	0,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,		0},
		D3DDECL_END()
	};
	V(device->CreateVertexDeclaration(Pos, &vertex_decl[VDI_POS]));*/
}

//=================================================================================================
void Render::Draw(bool call_present)
{
	/*HRESULT hr = device->TestCooperativeLevel();
	if(hr != D3D_OK)
	{
		lost_device = true;
		if(hr == D3DERR_DEVICELOST)
		{
			// device lost, can't reset yet
			Sleep(1);
			return;
		}
		else if(hr == D3DERR_DEVICENOTRESET)
		{
			// try reset
			if(!Reset(false))
			{
				Sleep(1);
				return;
			}
		}
		else
			throw Format("Render: Lost directx device (%d).", hr);
	}

	app::app->OnDraw();

	if(call_present)
	{
		hr = device->Present(nullptr, nullptr, app::engine->GetWindowHandle(), nullptr);
		if(FAILED(hr))
		{
			if(hr == D3DERR_DEVICELOST)
				lost_device = true;
			else
				throw Format("Render: Failed to present screen (%d).", hr);
		}
	}*/

	rot += t.Tick() * 3;

	device_context->ClearRenderTargetView(render_target, (float*)Vec4(0.1f, 0.1f, 0.1f, 1));
	device_context->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH, 1.f, 0);

	device_context->IASetInputLayout(layout);
	device_context->VSSetShader(vertex_shader, nullptr, 0);
	device_context->PSSetShader(pixel_shader, nullptr, 0);
	device_context->PSSetSamplers(0, 1, &sampler);
	device_context->PSSetShaderResources(0, 1, &tex->view);

	Matrix mat_world = Matrix::RotationY(rot),
		mat_view = Matrix::CreateLookAt(Vec3(0, 0, -2), Vec3(0, 0, 0)),
		mat_proj = Matrix::CreatePerspectiveFieldOfView(PI / 4, 1024.f / 768, 0.1f, 50.f),
		mat_combined = mat_world * mat_view * mat_proj;

	D3D11_MAPPED_SUBRESOURCE resource;
	V(device_context->Map(vs_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	Matrix& g = *(Matrix*)resource.pData;
	g = mat_combined.Transpose();
	device_context->Unmap(vs_buffer, 0);
	device_context->VSSetConstantBuffers(0, 1, &vs_buffer);

	uint stride = sizeof(Vec3) + sizeof(Vec2) + sizeof(Vec4),
		offset = 0;
	device_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

	device_context->Draw(6, 0);

	V(swap_chain->Present(vsync ? 1 : 0, 0));
}

//=================================================================================================
void Render::RegisterShader(ShaderHandler* shader)
{
	assert(shader);
	shaders.push_back(shader);
	shader->OnInit();
}

//=================================================================================================
/*ID3DXEffect* Render::CompileShader(cstring name)
{
	assert(name);

	CompileShaderParams params = { name };

	// add c to extension
	LocalString str = (shader_version == 3 ? "3_" : "2_");
	str += name;
	str += 'c';
	params.cache_name = str;

	// set shader version
	D3DXMACRO macros[3] = {
		"VS_VERSION", shader_version == 3 ? "vs_3_0" : "vs_2_0",
		"PS_VERSION", shader_version == 3 ? "ps_3_0" : "ps_2_0",
		nullptr, nullptr
	};
	params.macros = macros;

	return CompileShader(params);
}

//=================================================================================================
ID3DXEffect* Render::CompileShader(CompileShaderParams& params)
{
	assert(params.name && params.cache_name);

	ID3DXBuffer* errors = nullptr;
	ID3DXEffectCompiler* compiler = nullptr;
	cstring filename = Format("%s/%s", shaders_dir.c_str(), params.name);
	cstring cache_path = Format("cache/%s", params.cache_name);
	HRESULT hr;

	const DWORD flags =
#ifdef _DEBUG
		D3DXSHADER_DEBUG | D3DXSHADER_OPTIMIZATION_LEVEL1;
#else
		D3DXSHADER_OPTIMIZATION_LEVEL3;
#endif

	// open file and get date if not from string
	FileReader file;
	if(!params.input)
	{
		if(!file.Open(filename))
			throw Format("Render: Failed to load shader '%s' (%d).", params.name, GetLastError());
		params.file_time = file.GetTime();
	}

	// check if in cache
	{
		FileReader cache_file(cache_path);
		if(cache_file && params.file_time == cache_file.GetTime())
		{
			// same last modify time, use cache
			cache_file.ReadToString(g_tmp_string);
			ID3DXEffect* effect = nullptr;
			hr = D3DXCreateEffect(device, g_tmp_string.c_str(), g_tmp_string.size(), params.macros, nullptr, flags, params.pool, &effect, &errors);
			if(FAILED(hr))
			{
				Error("Render: Failed to create effect from cache '%s' (%d).\n%s", params.cache_name, hr,
					errors ? (cstring)errors->GetBufferPointer() : "No errors information.");
				SafeRelease(errors);
				SafeRelease(effect);
			}
			else
			{
				SafeRelease(errors);
				return effect;
			}
		}
	}

	// load from file
	if(!params.input)
	{
		file.ReadToString(g_tmp_string);
		params.input = &g_tmp_string;
	}
	hr = D3DXCreateEffectCompiler(params.input->c_str(), params.input->size(), params.macros, nullptr, flags, &compiler, &errors);
	if(FAILED(hr))
	{
		cstring str;
		if(errors)
			str = (cstring)errors->GetBufferPointer();
		else
		{
			switch(hr)
			{
			case D3DXERR_INVALIDDATA:
				str = "Invalid data.";
				break;
			case D3DERR_INVALIDCALL:
				str = "Invalid call.";
				break;
			case E_OUTOFMEMORY:
				str = "Out of memory.";
				break;
			case ERROR_MOD_NOT_FOUND:
			case 0x8007007e:
				str = "Can't find module (missing d3dcompiler_43.dll?).";
				break;
			default:
				str = "Unknown error.";
				break;
			}
		}

		cstring msg = Format("Render: Failed to compile shader '%s' (%d).\n%s", params.name, hr, str);

		SafeRelease(errors);

		throw msg;
	}
	SafeRelease(errors);

	// compile shader
	ID3DXBuffer* effect_buffer = nullptr;
	hr = compiler->CompileEffect(flags, &effect_buffer, &errors);
	if(FAILED(hr))
	{
		cstring msg = Format("Render: Failed to compile effect '%s' (%d).\n%s", params.name, hr,
			errors ? (cstring)errors->GetBufferPointer() : "No errors information.");

		SafeRelease(errors);
		SafeRelease(effect_buffer);
		SafeRelease(compiler);

		throw msg;
	}
	SafeRelease(errors);

	// save to cache
	io::CreateDirectory("cache");
	FileWriter f(cache_path);
	if(f)
	{
		f.Write(effect_buffer->GetBufferPointer(), effect_buffer->GetBufferSize());
		f.SetTime(params.file_time);
	}
	else
		Warn("Render: Failed to save effect '%s' to cache (%d).", params.cache_name, GetLastError());

	// create effect from effect buffer
	ID3DXEffect* effect = nullptr;
	hr = D3DXCreateEffect(device, effect_buffer->GetBufferPointer(), effect_buffer->GetBufferSize(),
		params.macros, nullptr, flags, params.pool, &effect, &errors);
	if(FAILED(hr))
	{
		cstring msg = Format("Render: Failed to create effect '%s' (%d).\n%s", params.name, hr,
			errors ? (cstring)errors->GetBufferPointer() : "No errors information.");

		SafeRelease(errors);
		SafeRelease(effect_buffer);
		SafeRelease(compiler);

		throw msg;
	}

	// free directx stuff
	SafeRelease(errors);
	SafeRelease(effect_buffer);
	SafeRelease(compiler);

	return effect;
}

//=================================================================================================
TEX Render::CreateTexture(const Int2& size)
{
	TEX tex;
	V(device->CreateTexture(size.x, size.y, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, nullptr));
	return tex;
}*/

//=================================================================================================
//RenderTarget* Render::CreateRenderTarget(const Int2& size)
//{
//	assert(size.x > 0 && size.y > 0 && IsPow2(size.x) && IsPow2(size.y));
//	RenderTarget* target = new RenderTarget;
//	target->size = size;
//	CreateRenderTargetTexture(target);
//	targets.push_back(target);
//	return target;
//}

//=================================================================================================
void Render::CreateRenderTargetTexture(RenderTarget* target)
{
	/*V(device->CreateTexture(target->size.x, target->size.y, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &target->tex.tex, nullptr));
	D3DMULTISAMPLE_TYPE type = (D3DMULTISAMPLE_TYPE)multisampling;
	if(type != D3DMULTISAMPLE_NONE)
		V(device->CreateRenderTarget(target->size.x, target->size.y, D3DFMT_A8R8G8B8, type, multisampling_quality, FALSE, &target->surf, nullptr));
	else
		target->surf = nullptr;
	target->tex.state = ResourceState::Loaded;*/
}

//=================================================================================================
//Texture* Render::CopyToTexture(RenderTarget* target)
//{
//	assert(target);
//	D3DSURFACE_DESC desc;
//	V(target->tex.tex->GetLevelDesc(0, &desc));
//	TEX tex;
//	V(device->CreateTexture(desc.Width, desc.Height, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, nullptr));
//	SURFACE surf;
//	V(tex->GetSurfaceLevel(0, &surf));
//	HRESULT hr = D3DXLoadSurfaceFromSurface(surf, nullptr, nullptr, target->GetSurface(), nullptr, nullptr, D3DX_DEFAULT, 0);
//	target->FreeSurface();
//	surf->Release();
//	if(SUCCEEDED(hr))
//	{
//		Texture* t = new Texture;
//		t->tex = tex;
//		t->state = ResourceState::Loaded;
//		return t;
//	}
//	else
//	{
//		tex->Release();
//		WaitReset();
//		return nullptr;
//	}
//}

//=================================================================================================
void Render::SetAlphaBlend(bool use_alphablend)
{
	if(use_alphablend != r_alphablend)
	{
		r_alphablend = use_alphablend;
		//V(device->SetRenderState(D3DRS_ALPHABLENDENABLE, r_alphablend ? TRUE : FALSE));
	}
}

//=================================================================================================
void Render::SetAlphaTest(bool use_alphatest)
{
	if(use_alphatest != r_alphatest)
	{
		r_alphatest = use_alphatest;
		//V(device->SetRenderState(D3DRS_ALPHATESTENABLE, r_alphatest ? TRUE : FALSE));
	}
}

//=================================================================================================
void Render::SetNoCulling(bool use_nocull)
{
	if(use_nocull != r_nocull)
	{
		r_nocull = use_nocull;
		//V(device->SetRenderState(D3DRS_CULLMODE, r_nocull ? D3DCULL_NONE : D3DCULL_CCW));
	}
}

//=================================================================================================
void Render::SetNoZWrite(bool use_nozwrite)
{
	if(use_nozwrite != r_nozwrite)
	{
		r_nozwrite = use_nozwrite;
		//V(device->SetRenderState(D3DRS_ZWRITEENABLE, r_nozwrite ? FALSE : TRUE));
	}
}

//=================================================================================================
int Render::SetMultisampling(int type, int level)
{
	/*if(type == multisampling && (level == -1 || level == multisampling_quality))
		return 1;

	if(!initialized)
	{
		multisampling = type;
		multisampling_quality = level;
		return 2;
	}

	bool fullscreen = app::engine->IsFullscreen();
	DWORD levels, levels2;
	if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(0, D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8, fullscreen ? FALSE : TRUE, (D3DMULTISAMPLE_TYPE)type, &levels))
		&& SUCCEEDED(d3d->CheckDeviceMultiSampleType(0, D3DDEVTYPE_HAL, D3DFMT_D24S8, fullscreen ? FALSE : TRUE, (D3DMULTISAMPLE_TYPE)type, &levels2)))
	{
		levels = min(levels, levels2);
		if(level < 0)
			level = levels - 1;
		else if(level >= (int)levels)
			return 0;

		multisampling = type;
		multisampling_quality = level;

		Reset(true);

		return 2;
	}
	else
		return 0;*/
	return 0;
}

//=================================================================================================
void Render::GetResolutions(vector<Resolution>& v) const
{
	/*v.clear();
	uint display_modes = d3d->GetAdapterModeCount(used_adapter, DISPLAY_FORMAT);
	for(uint i = 0; i < display_modes; ++i)
	{
		D3DDISPLAYMODE d_mode;
		V(d3d->EnumAdapterModes(used_adapter, DISPLAY_FORMAT, i, &d_mode));
		if(d_mode.Width >= (uint)Engine::MIN_WINDOW_SIZE.x && d_mode.Height >= (uint)Engine::MIN_WINDOW_SIZE.y)
			v.push_back({ Int2(d_mode.Width, d_mode.Height), d_mode.RefreshRate });
	}*/
}

//=================================================================================================
void Render::GetMultisamplingModes(vector<Int2>& v) const
{
	/*v.clear();
	for(int j = 2; j <= 16; ++j)
	{
		DWORD levels, levels2;
		if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, BACKBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels))
			&& SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, ZBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels2)))
		{
			int level = min(levels, levels2);
			for(int i = 0; i < level; ++i)
				v.push_back(Int2(j, i));
		}
	}*/
}

//=================================================================================================
void Render::SetTarget(RenderTarget* target)
{
	//if(target)
	//{
	//	assert(!current_target);

	//	if(target->surf)
	//		V(device->SetRenderTarget(0, target->surf));
	//	else
	//	{
	//		V(target->tex.tex->GetSurfaceLevel(0, &current_surf));
	//		V(device->SetRenderTarget(0, current_surf));
	//	}

	//	current_target = target;
	//}
	//else
	//{
	//	assert(current_target);

	//	if(current_target->tmp_surf)
	//	{
	//		current_target->surf->Release();
	//		current_target->surf = nullptr;
	//		current_target->tmp_surf = false;
	//	}
	//	else
	//	{
	//		// copy to surface if using multisampling
	//		if(current_target->surf)
	//		{
	//			V(current_target->tex.tex->GetSurfaceLevel(0, &current_surf));
	//			V(device->StretchRect(current_target->surf, nullptr, current_surf, nullptr, D3DTEXF_NONE));
	//		}
	//		current_surf->Release();
	//	}

	//	// restore old render target
	//	V(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &current_surf));
	//	V(device->SetRenderTarget(0, current_surf));
	//	current_surf->Release();

	//	current_target = nullptr;
	//	current_surf = nullptr;
	//}
}

//=================================================================================================
void Render::SetTextureAddressMode(TextureAddressMode mode)
{
	/*V(device->SetSamplerState(0, D3DSAMP_ADDRESSU, (D3DTEXTUREADDRESS)mode));
	V(device->SetSamplerState(0, D3DSAMP_ADDRESSV, (D3DTEXTUREADDRESS)mode));*/
}

void Render::Init2()
{
	ID3DBlob* vs_buf = CompileShader("test.hlsl", "vs_main", true);
	HRESULT result = device->CreateVertexShader(vs_buf->GetBufferPointer(), vs_buf->GetBufferSize(), nullptr, &vertex_shader);
	//if(FAILED(result))
	//	throw Format("Failed to create vertex shader '%s' (%u).", filename, result);

	ID3DBlob* ps_buf = CompileShader("test.hlsl", "ps_main", false);
	result = device->CreatePixelShader(ps_buf->GetBufferPointer(), ps_buf->GetBufferSize(), nullptr, &pixel_shader);
	//if(FAILED(result))
	//	throw Format("Failed to create pixel shader '%s' (%u).", filename, result);

	// create layout
	D3D11_INPUT_ELEMENT_DESC desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	result = device->CreateInputLayout(desc, countof(desc), vs_buf->GetBufferPointer(), vs_buf->GetBufferSize(), &layout);
	//if(FAILED(result))
	//	throw Format("Failed to create input layout '%s' (%u).", filename, result);

	vs_buffer = CreateConstantBuffer(sizeof(Matrix));

	vs_buf->Release();
	ps_buf->Release();

	// create mesh

	struct VerTex
	{
		Vec3 pos;
		Vec2 tex;
		Vec4 color;
	};
	VerTex data[6] = {
		{ Vec3(-0.5f, -0.5f, 0.f), Vec2(0,0), Vec4(1,0,0,1)},
		{ Vec3(0,0.5f,0), Vec2(0.5f,1), Vec4(0,1,0,1)},
		{ Vec3(0.5f,-0.5f,0), Vec2(1,0), Vec4(0,0,1,1)},
		{ Vec3(-0.5f, -0.5f, 0.f), Vec2(0,0), Vec4(1,0,0,1)},
		{ Vec3(0.5f,-0.5f,0), Vec2(1,0), Vec4(0,0,1,1)},
		{ Vec3(0,0.5f,0), Vec2(0.5f,1), Vec4(0,1,0,1)}
	};

	D3D11_BUFFER_DESC v_desc;
	v_desc.Usage = D3D11_USAGE_DEFAULT;
	v_desc.ByteWidth = sizeof(data);
	v_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	v_desc.CPUAccessFlags = 0;
	v_desc.MiscFlags = 0;
	v_desc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA v_data;
	v_data.pSysMem = data;

	result = device->CreateBuffer(&v_desc, &v_data, &vb);
	if(FAILED(result))
		throw Format("Failed to create vertex buffer (%u).", result);

	app::res_mgr->AddDir("data");
	tex = app::res_mgr->Load<Texture>("woodmgrid1a.jpg");

	// create texture sampler
	D3D11_SAMPLER_DESC sampler_desc;
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.MipLODBias = 0.0f;
	sampler_desc.MaxAnisotropy = 1;
	sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sampler_desc.BorderColor[0] = 0;
	sampler_desc.BorderColor[1] = 0;
	sampler_desc.BorderColor[2] = 0;
	sampler_desc.BorderColor[3] = 0;
	sampler_desc.MinLOD = 0;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

	result = device->CreateSamplerState(&sampler_desc, &sampler);
	if(FAILED(result))
		throw Format("Failed to create sampler state (%u).", result);

	// over and out

	t.Start();
	rot = 0;
}


ID3DBlob* Render::CompileShader(cstring filename, cstring entry, bool is_vertex)
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
	HRESULT result = D3DCompileFromFile(ToWString(path), nullptr, nullptr, entry, target, flags, 0, &shader_blob, &error_blob);
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


ID3D11Buffer* Render::CreateConstantBuffer(uint size)
{
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
