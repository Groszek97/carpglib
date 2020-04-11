#include "Pch.h"
#include "Render.h"

#include "DirectX.h"
#include "Engine.h"
#include "File.h"
#include "RenderTarget.h"
#include "ScreenGrab.h"
#include "ShaderHandler.h"
#include "VertexDeclaration.h"

#include <d3dcompiler.h>
#include <wincodec.h>

Render* app::render;
static const DXGI_FORMAT DISPLAY_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;

//=================================================================================================
Render::Render() : initialized(false), vsync(true), shaders_dir("shaders"), refreshHz(0), usedAdapter(0), multisampling(0), multisamplingQuality(0),
factory(nullptr), adapter(nullptr), swapChain(nullptr), device(nullptr), deviceContext(nullptr), renderTarget(nullptr), depthStencilView(nullptr),
blendStates(), depthStates(), rasterStates(), blendState(BLEND_NO), depthState(DEPTH_YES), rasterState(RASTER_NORMAL)
{
}

//=================================================================================================
Render::~Render()
{
	for(ShaderHandler* shader : shaders)
	{
		shader->OnRelease();
		delete shader;
	}
	DeleteElements(renderTargets);

	SafeRelease(depthStencilView);
	SafeRelease(renderTarget);
	SafeRelease(swapChain);
	SafeRelease(deviceContext);
	SafeRelease(blendStates);
	SafeRelease(depthStates);
	SafeRelease(rasterStates);

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
	wndSize = app::engine->GetWindowSize();

	CreateAdapter();
	LogAndSelectResolution();
	CreateDeviceAndSwapChain();
	CreateSizeDependentResources();
	CreateBlendStates();
	CreateDepthStates();
	CreateRasterStates();

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	/*
	// check multisampling
	DWORD levels, levels2;
	if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(usedAdapter, D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8, fullscreen ? FALSE : TRUE,
		(D3DMULTISAMPLE_TYPE)multisampling, &levels))
		&& SUCCEEDED(d3d->CheckDeviceMultiSampleType(usedAdapter, D3DDEVTYPE_HAL, D3DFMT_D24S8, fullscreen ? FALSE : TRUE,
		(D3DMULTISAMPLE_TYPE)multisampling, &levels2)))
	{
		levels = min(levels, levels2);
		if(multisamplingQuality < 0 || multisamplingQuality >= (int)levels)
		{
			Warn("Render: Unavailable multisampling quality, changed to 0.");
			multisamplingQuality = 0;
		}
	}
	else
	{
		Warn("Render: Your graphic card don't support multisampling x%d. Maybe it's only available in fullscreen mode. "
			"Multisampling was turned off.", multisampling);
		multisampling = 0;
		multisamplingQuality = 0;
	}

	LogMultisampling();
	LogAndSelectResolution();
;*/

	initialized = true;
	Info("Render: Directx device created.");
}

//=================================================================================================
void Render::CreateAdapter()
{
	// create dxgi factory
	V(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory));

	// enumerate adapters
	IDXGIAdapter* tmp_adapter;
	for(int i = 0; factory->EnumAdapters(i, &tmp_adapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC desc;
		V(tmp_adapter->GetDesc(&desc));
		Info("Render: Adapter %d: %s", i, ToString(desc.Description));
		if(usedAdapter == i)
			adapter = tmp_adapter;
		else
			tmp_adapter->Release();
	}

	// fallback to first adapter
	if(!adapter)
	{
		Warn("Render: Invalid adapter %d, defaulting to 0.", usedAdapter);
		usedAdapter = 0;
		V(factory->EnumAdapters(0, &adapter));
	}
}

//=================================================================================================
void Render::CreateDeviceAndSwapChain()
{
	DXGI_SWAP_CHAIN_DESC swap_desc = {};
	swap_desc.BufferCount = 1;
	swap_desc.BufferDesc.Width = wndSize.x;
	swap_desc.BufferDesc.Height = wndSize.y;
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
		D3D11_SDK_VERSION, &swap_desc, &swapChain, &device, &feature_level, &deviceContext);
	if(FAILED(result))
		throw Format("Failed to create device and swap chain (%u).", result);

	// disable builtin alt+enter
	V(factory->MakeWindowAssociation(swap_desc.OutputWindow, DXGI_MWA_NO_WINDOW_CHANGES));
}

//=================================================================================================
void Render::CreateSizeDependentResources()
{
	CreateRenderTarget();
	CreateDepthStencilView();
	deviceContext->OMSetRenderTargets(1, &renderTarget, depthStencilView);
	SetViewport();
}

//=================================================================================================
void Render::CreateRenderTarget()
{
	HRESULT result;
	ID3D11Texture2D* back_buffer;
	result = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer);
	if(FAILED(result))
		throw Format("Failed to get back buffer (%u).", result);

	// Create the render target view with the back buffer pointer.
	result = device->CreateRenderTargetView(back_buffer, NULL, &renderTarget);
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

	tex_desc.Width = wndSize.x;
	tex_desc.Height = wndSize.y;
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

	// create depth stencil view from texture
	D3D11_DEPTH_STENCIL_VIEW_DESC view_desc = {};

	view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	view_desc.Texture2D.MipSlice = 0;

	V(device->CreateDepthStencilView(depth_tex, &view_desc, &depthStencilView));

	depth_tex->Release();
}

//=================================================================================================
void Render::SetViewport()
{
	D3D11_VIEWPORT viewport;
	viewport.Width = (float)wndSize.x;
	viewport.Height = (float)wndSize.y;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	deviceContext->RSSetViewports(1, &viewport);
}

//=================================================================================================
void Render::CreateBlendStates()
{
	// get disabled blend state
	deviceContext->OMGetBlendState(&blendStates[BLEND_NO], nullptr, nullptr);

	// create additive blend state
	D3D11_BLEND_DESC desc = {};
	desc.RenderTarget[0].BlendEnable = true;
	desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	V(device->CreateBlendState(&desc, &blendStates[BLEND_ADD]));

	// create additive one blend state
	desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	V(device->CreateBlendState(&desc, &blendStates[BLEND_ADD_ONE]));

	// create reverse subtract blend state
	desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_REV_SUBTRACT;
	V(device->CreateBlendState(&desc, &blendStates[BLEND_REV_SUBTRACT]));
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

	V(device->CreateDepthStencilState(&desc, &depthStates[DEPTH_YES]));
	deviceContext->OMSetDepthStencilState(depthStates[DEPTH_YES], 1);

	// create depth stencil state with disabled depth test
	desc.DepthEnable = false;
	desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	V(device->CreateDepthStencilState(&desc, &depthStates[DEPTH_NO]));

	// create readonly depth stencil state
	desc.DepthEnable = true;
	desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	V(device->CreateDepthStencilState(&desc, &depthStates[DEPTH_READ]));

	// create stencil enabled depth stencil state
	desc.StencilEnable = true;
	desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	desc.BackFace = desc.FrontFace;

	V(device->CreateDepthStencilState(&desc, &depthStates[DEPTH_USE_STENCIL]));
}

//=================================================================================================
void Render::CreateRasterStates()
{
	// get normal raster state
	deviceContext->RSGetState(&rasterStates[RASTER_NORMAL]);

	// create disabled culling raster state
	D3D11_RASTERIZER_DESC desc = {};
	desc.FillMode = D3D11_FILL_SOLID;
	desc.CullMode = D3D11_CULL_NONE;
	desc.DepthClipEnable = true;

	V(device->CreateRasterizerState(&desc, &rasterStates[RASTER_NO_CULLING]));

	// create wireframe raster state
	desc.FillMode = D3D11_FILL_WIREFRAME;

	V(device->CreateRasterizerState(&desc, &rasterStates[RASTER_WIREFRAME]));
}

//=================================================================================================
void Render::LogMultisampling()
{
	LocalString s = "Render: Available multisampling: ";

	/*for(int j = 2; j <= 16; ++j)
	{
		DWORD levels, levels2;
		if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(usedAdapter, D3DDEVTYPE_HAL, BACKBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels))
			&& SUCCEEDED(d3d->CheckDeviceMultiSampleType(usedAdapter, D3DDEVTYPE_HAL, ZBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels2)))
		{
			s += Format("x%d(%d), ", j, min(levels, levels2));
		}
	}*/

	if(s.at_back(1) == ':')
		s += "none";
	else
		s.pop(2);

	Info(s);
}

//=================================================================================================
void Render::LogAndSelectResolution()
{
	// enum display modes
	IDXGIOutput* output;
	V(adapter->EnumOutputs(0, &output));

	uint count;
	V(output->GetDisplayModeList(DISPLAY_FORMAT, 0, &count, nullptr));

	Buf buf;
	DXGI_MODE_DESC* modes = buf.Get<DXGI_MODE_DESC>(sizeof(DXGI_MODE_DESC) * count);
	V(output->GetDisplayModeList(DISPLAY_FORMAT, 0, &count, modes));

	output->Release();

	// list all resolutions
	Int2 wndSize = app::engine->GetWindowSize();
	int bestDefHz = 0, bestHz = 0;
	bool resValid = false, hzValid = false;
	for(uint i = 0; i < count; ++i)
	{
		DXGI_MODE_DESC& mode = modes[i];
		if(mode.Width < (uint)Engine::MIN_WINDOW_SIZE.x || mode.Height < (uint)Engine::MIN_WINDOW_SIZE.y)
			continue;
		uint hz = mode.RefreshRate.Numerator / mode.RefreshRate.Denominator;
		resolutions.push_back({ Int2(mode.Width, mode.Height), hz });
		if(mode.Width == (uint)Engine::DEFAULT_WINDOW_SIZE.x && mode.Height == (uint)Engine::DEFAULT_WINDOW_SIZE.y)
		{
			if(hz > (uint)bestDefHz)
				bestDefHz = hz;
		}
		if(mode.Width == wndSize.x && mode.Height == wndSize.y)
		{
			resValid = true;
			if(hz == refreshHz)
				hzValid = true;
			if((int)hz > bestHz)
				bestHz = hz;
		}
	}
	std::sort(resolutions.begin(), resolutions.end());

	// pretty print
	int cw = 0, ch = 0;
	LocalString str = "Render: Available display modes:";
	for(vector<Resolution>::iterator it = resolutions.begin(), end = resolutions.end(); it != end; ++it)
	{
		Resolution& r = *it;
		if(r.size.x != cw || r.size.y != ch)
		{
			if(it != resolutions.begin())
				str += " Hz)";
			str += Format("\n\t%dx%d (%u", r.size.x, r.size.y, r.hz);
			cw = r.size.x;
			ch = r.size.y;
		}
		else
			str += Format(", %d", r.hz);
	}
	str += " Hz)";
	Info(str->c_str());

	// adjust selected resolution
	if(!resValid)
	{
		if(wndSize.x != 0)
		{
			Warn("Render: Resolution %dx%d is not valid, defaulting to %dx%d (%d Hz).", wndSize.x, wndSize.y,
				Engine::DEFAULT_WINDOW_SIZE.x, Engine::DEFAULT_WINDOW_SIZE.y, bestDefHz);
		}
		else
			Info("Render: Defaulting resolution to %dx%dx (%d Hz).", Engine::DEFAULT_WINDOW_SIZE.x, Engine::DEFAULT_WINDOW_SIZE.y, bestDefHz);
		refreshHz = bestDefHz;
		app::engine->SetWindowSizeInternal(Engine::DEFAULT_WINDOW_SIZE);
	}
	else if(!hzValid)
	{
		if(refreshHz != 0)
			Warn("Render: Refresh rate %d Hz is not valid, defaulting to %d Hz.", refreshHz, bestHz);
		else
			Info("Render: Defaulting refresh rate to %d Hz.", bestHz);
		refreshHz = bestHz;
	}
}

//=================================================================================================
void Render::Clear(const Vec4& color)
{
	deviceContext->ClearRenderTargetView(renderTarget, (const float*)color);
	deviceContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.f, 0);
}

//=================================================================================================
void Render::Present()
{
	V(swapChain->Present(vsync ? 1 : 0, 0));
}

//=================================================================================================
bool Render::CheckDisplay(const Int2& size, uint& hz)
{
	FIXME;
	/*assert(size.x >= Engine::MIN_WINDOW_SIZE.x && size.x >= Engine::MIN_WINDOW_SIZE.y);

	// check minimum resolution
	if(size.x < Engine::MIN_WINDOW_SIZE.x || size.y < Engine::MIN_WINDOW_SIZE.y)
		return false;

	uint display_modes = d3d->GetAdapterModeCount(usedAdapter, DISPLAY_FORMAT);

	if(hz == 0)
	{
		bool valid = false;

		for(uint i = 0; i < display_modes; ++i)
		{
			D3DDISPLAYMODE d_mode;
			V(d3d->EnumAdapterModes(usedAdapter, DISPLAY_FORMAT, i, &d_mode));
			if(size.x == d_mode.Width && size.y == d_mode.Height)
			{
				valid = true;
				if(hz < (int)d_mode.RefreshRate)
					hz = d_mode.RefreshRate;
			}
		}

		return valid;
	}
	else
	{
		for(uint i = 0; i < display_modes; ++i)
		{
			D3DDISPLAYMODE d_mode;
			V(d3d->EnumAdapterModes(usedAdapter, DISPLAY_FORMAT, i, &d_mode));
			if(size.x == d_mode.Width && size.y == d_mode.Height && hz == d_mode.RefreshRate)
				return true;
		}

		return false;
	}*/

	return false;
}

//=================================================================================================
void Render::RegisterShader(ShaderHandler* shader)
{
	assert(shader);
	shaders.push_back(shader);
	shader->OnInit();
}

//=================================================================================================
void Render::ReloadShaders()
{
	Info("Reloading shaders...");

	for(ShaderHandler* shader : shaders)
		shader->OnRelease();

	try
	{
		for(ShaderHandler* shader : shaders)
			shader->OnInit();
	}
	catch(cstring err)
	{
		app::engine->FatalError(Format("Failed to reload shaders.\n%s", err));
		return;
	}
}

//=================================================================================================
TEX Render::CreateRawTexture(const Int2& size, const Color* fill, bool allowMipmaps)
{
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = size.x;
	desc.Height = size.y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	if(allowMipmaps)
	{
		desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
		desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	}

	ID3D11Texture2D* tex;
	if(fill)
	{
		desc.Usage = D3D11_USAGE_IMMUTABLE;

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

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
	viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipLevels = 1;

	ID3D11ShaderResourceView* view;
	V(device->CreateShaderResourceView(tex, &viewDesc, &view));
	tex->Release();

	return view;
}

//=================================================================================================
Texture* Render::CreateTexture(const Int2& size)
{
	Texture* tex = new Texture;
	tex->tex = CreateRawTexture(size);
	tex->state = ResourceState::Loaded;
	return tex;
}

//=================================================================================================
RenderTarget* Render::CreateRenderTarget(const Int2& size)
{
	assert(size <= wndSize);
	assert(size.x > 0 && size.y > 0 && IsPow2(size.x) && IsPow2(size.y));
	RenderTarget* target = new RenderTarget;
	target->size = size;
	target->tex = CreateRawTexture(size);
	renderTargets.push_back(target);
	return target;
}

//=================================================================================================
Texture* Render::CopyToTexture(RenderTarget* target)
{
	assert(target);

	Texture* tex = new Texture;
	tex->tex = CopyToTextureRaw(target);
	tex->state = ResourceState::Loaded;
	return tex;
}

//=================================================================================================
TEX Render::CopyToTextureRaw(RenderTarget* target)
{
	assert(target);

	// get render target texture data
	ID3D11Texture2D* res;
	target->tex->GetResource(reinterpret_cast<ID3D11Resource**>(&res));

	D3D11_TEXTURE2D_DESC texDesc;
	res->GetDesc(&texDesc);

	// create new texture
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = texDesc.Width;
	desc.Height = texDesc.Height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = 0;

	ID3D11Texture2D* tex;
	V(device->CreateTexture2D(&desc, nullptr, &tex));

	// copy texture
	deviceContext->CopyResource(tex, res);

	// create view
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
	viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipLevels = 1;

	ID3D11ShaderResourceView* view;
	V(device->CreateShaderResourceView(tex, &viewDesc, &view));

	tex->Release();
	res->Release();
	return view;
}

//=================================================================================================
void Render::SetBlendState(BlendState blendState)
{
	assert(blendState >= 0 && blendState < BLEND_MAX);
	if(this->blendState != blendState)
	{
		this->blendState = blendState;
		deviceContext->OMSetBlendState(blendStates[blendState], nullptr, 0xFFFFFFFF);
	}
}

//=================================================================================================
void Render::SetDepthState(DepthState depthState)
{
	assert(depthState >= 0 && depthState < DEPTH_MAX);
	if(this->depthState != depthState)
	{
		this->depthState = depthState;
		deviceContext->OMSetDepthStencilState(depthStates[depthState], 1);
	}
}

//=================================================================================================
void Render::SetRasterState(RasterState rasterState)
{
	assert(rasterState >= 0 && rasterState < RASTER_MAX);
	if(this->rasterState != rasterState)
	{
		this->rasterState = rasterState;
		deviceContext->RSSetState(rasterStates[rasterState]);
	}
}

//=================================================================================================
int Render::SetMultisampling(int type, int level)
{
	FIXME;
	/*if(type == multisampling && (level == -1 || level == multisamplingQuality))
		return 1;

	if(!initialized)
	{
		multisampling = type;
		multisamplingQuality = level;
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
		multisamplingQuality = level;

		Reset(true);

		return 2;
	}
	else
		return 0;*/
	return 0;
}

//=================================================================================================
void Render::GetMultisamplingModes(vector<Int2>& v) const
{
	FIXME;
	/*v.clear();
	for(int j = 2; j <= 16; ++j)
	{
		DWORD levels, levels2;
		if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(usedAdapter, D3DDEVTYPE_HAL, BACKBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels))
			&& SUCCEEDED(d3d->CheckDeviceMultiSampleType(usedAdapter, D3DDEVTYPE_HAL, ZBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels2)))
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
	FIXME;
	/*if(target)
	{
		assert(!current_target);

		if(target->surf)
			V(device->SetRenderTarget(0, target->surf));
		else
		{
			V(target->tex->GetSurfaceLevel(0, &current_surf));
			V(device->SetRenderTarget(0, current_surf));
		}

		current_target = target;
	}
	else
	{
		assert(current_target);

		if(current_target->tmp_surf)
		{
			current_target->surf->Release();
			current_target->surf = nullptr;
			current_target->tmp_surf = false;
		}
		else
		{
			// copy to surface if using multisampling
			if(current_target->surf)
			{
				V(current_target->tex->GetSurfaceLevel(0, &current_surf));
				V(device->StretchRect(current_target->surf, nullptr, current_surf, nullptr, D3DTEXF_NONE));
			}
			current_surf->Release();
		}

		// restore old render target
		V(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &current_surf));
		V(device->SetRenderTarget(0, current_surf));
		current_surf->Release();

		current_target = nullptr;
		current_surf = nullptr;
	}*/
}

//=================================================================================================
ID3D11Buffer* Render::CreateConstantBuffer(uint size, cstring name)
{
	size = alignto(size, 16);

	D3D11_BUFFER_DESC cbDesc;
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.ByteWidth = size;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	ID3D11Buffer* buffer;
	HRESULT result = device->CreateBuffer(&cbDesc, NULL, &buffer);
	if(FAILED(result))
		throw Format("Failed to create constant buffer (size:%u; code:%u).", size, result);

	if(name)
		SetDebugName(buffer, name);

	return buffer;
}

//=================================================================================================
ID3D11SamplerState* Render::CreateSampler(TextureAddressMode mode, bool disableMipmap)
{
	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = (D3D11_TEXTURE_ADDRESS_MODE)mode;
	samplerDesc.AddressV = (D3D11_TEXTURE_ADDRESS_MODE)mode;
	samplerDesc.AddressW = (D3D11_TEXTURE_ADDRESS_MODE)mode;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = disableMipmap ? 0 : D3D11_FLOAT32_MAX;

	ID3D11SamplerState* sampler;
	HRESULT result = device->CreateSamplerState(&samplerDesc, &sampler);
	if(FAILED(result))
		throw Format("Failed to create sampler state (%u).", result);

	return sampler;
}

//=================================================================================================
void Render::CreateShader(cstring filename, VertexDeclarationId decl, ID3D11VertexShader*& vertexShader,
	ID3D11PixelShader*& pixelShader, ID3D11InputLayout*& layout, D3D_SHADER_MACRO* macro, cstring vsEntry, cstring psEntry)
{

	VertexDeclaration& vertDecl = VertexDeclaration::decl[(int)decl];

	try
	{
		CPtr<ID3DBlob> vsBuf = CompileShader(filename, vsEntry, true, macro);
		HRESULT result = device->CreateVertexShader(vsBuf->GetBufferPointer(), vsBuf->GetBufferSize(), nullptr, &vertexShader);
		if(FAILED(result))
			throw Format("Failed to create vertex shader (%u).", result);

		CPtr<ID3DBlob> psBuf = CompileShader(filename, psEntry, false, macro);
		result = device->CreatePixelShader(psBuf->GetBufferPointer(), psBuf->GetBufferSize(), nullptr, &pixelShader);
		if(FAILED(result))
			throw Format("Failed to create pixel shader (%u).", result);

		result = device->CreateInputLayout(vertDecl.desc, vertDecl.count, vsBuf->GetBufferPointer(), vsBuf->GetBufferSize(), &layout);
		if(FAILED(result))
			throw Format("Failed to create input layout (%u).", result);

		SetDebugName(vertexShader, Format("%s/%s", filename, vsEntry));
		SetDebugName(pixelShader, Format("%s/%s", filename, psEntry));
		SetDebugName(layout, Format("%s/layout", filename));
	}
	catch(cstring err)
	{
		throw Format("Failed to create shader '%s': %s", filename, err);
	}
}

//=================================================================================================
ID3D11VertexShader* Render::CreateVertexShader(cstring filename, cstring entry, ID3DBlob** vsBlob)
{
	try
	{
		ID3D11VertexShader* vertexShader;

		CPtr<ID3DBlob> vsBuf = CompileShader(filename, entry, true, nullptr);
		HRESULT result = device->CreateVertexShader(vsBuf->GetBufferPointer(), vsBuf->GetBufferSize(), nullptr, &vertexShader);
		if(FAILED(result))
			throw Format("Error %u.", result);

		SetDebugName(vertexShader, Format("%s/%s", filename, entry));

		if(vsBlob)
			*vsBlob = vsBuf.Pin();

		return vertexShader;
	}
	catch(cstring err)
	{
		throw Format("Failed to create vertex shader '%s': %s", filename, err);
	}
}

//=================================================================================================
ID3D11PixelShader* Render::CreatePixelShader(cstring filename, cstring entry)
{
	try
	{
		ID3D11PixelShader* pixelShader;

		CPtr<ID3DBlob> psBuf = CompileShader(filename, entry, false, nullptr);
		HRESULT result = device->CreatePixelShader(psBuf->GetBufferPointer(), psBuf->GetBufferSize(), nullptr, &pixelShader);
		if(FAILED(result))
			throw Format("Error %u.", result);

		SetDebugName(pixelShader, Format("%s/%s", filename, entry));

		return pixelShader;
	}
	catch(cstring err)
	{
		throw Format("Failed to create pixel shader '%s': %s", filename, err);
	}
}

//=================================================================================================
ID3DBlob* Render::CompileShader(cstring filename, cstring entry, bool isVertex, D3D_SHADER_MACRO* macro)
{
	assert(filename && entry);

	cstring target = isVertex ? "vs_5_0" : "ps_5_0";

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
			cstring msg = Format("Failed to compile function %s: %s (code %u).", entry, err, result);
			error_blob->Release();
			throw msg;
		}
		else if(result == D3D11_ERROR_FILE_NOT_FOUND || result == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
			throw Format("Failed to compile function %s: file not found.", entry);
		else
			throw Format("Failed to compile function %s (code %u).", entry, result);
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
ID3D11InputLayout* Render::CreateInputLayout(VertexDeclarationId decl, ID3DBlob* vsBlob, cstring name)
{
	ID3D11InputLayout* layout ;
	VertexDeclaration& vertDecl = VertexDeclaration::decl[(int)decl];

	HRESULT result = device->CreateInputLayout(vertDecl.desc, vertDecl.count, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &layout );
	if(FAILED(result))
		throw Format("Failed to create input layout (%u).", result);

	if(name)
		SetDebugName(layout, name);

	return layout ;
}

//=================================================================================================
void Render::SaveToFile(TEX tex, cstring path, ImageFormat format)
{
	ID3D11Texture2D* res;
	tex->GetResource(reinterpret_cast<ID3D11Resource**>(&res));

	GUID formatGuid;
	switch(format)
	{
	case ImageFormat::BMP:
		formatGuid = GUID_ContainerFormatBmp;
		break;
	case ImageFormat::JPG:
		formatGuid = GUID_ContainerFormatJpeg;
		break;
	case ImageFormat::TIF:
		formatGuid = GUID_ContainerFormatTiff;
		break;
	case ImageFormat::GIF:
		formatGuid = GUID_ContainerFormatGif;
		break;
	case ImageFormat::PNG:
		formatGuid = GUID_ContainerFormatPng;
		break;
	case ImageFormat::DDS:
		formatGuid = GUID_ContainerFormatDds;
		break;
	}

	V(SaveWICTextureToFile(deviceContext, res, formatGuid, ToWString(path)));

	res->Release();
}
