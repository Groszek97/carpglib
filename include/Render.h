#pragma once

//-----------------------------------------------------------------------------
class Render
{
	friend class SceneManager;
public:
	enum DepthState
	{
		DEPTH_YES,
		DEPTH_READONLY,
		DEPTH_NO,
		DEPTH_MAX
	};

	enum TextureAddressMode
	{
		TEX_ADR_WRAP = 1,
		TEX_ADR_MIRROR = 2,
		TEX_ADR_CLAMP = 3,
		TEX_ADR_BORDER = 4,
		TEX_ADR_MIRRORONCE = 5
	};

	Render();
	~Render();
	void Init();
	void Clear(const Vec4& color);
	void Present();
	void OnChangeResolution();

	TEX CreateTexture(const Int2& size, Color* fill);
	void CreateShader(cstring filename, D3D11_INPUT_ELEMENT_DESC* input, uint input_count, ID3D11VertexShader*& vertex_shader,
		ID3D11PixelShader*& pixel_shader, ID3D11InputLayout*& layout, D3D_SHADER_MACRO* macro = nullptr);
	ID3DBlob* CompileShader(cstring filename, cstring entry, bool is_vertex, D3D_SHADER_MACRO* macro);
	ID3D11Buffer* CreateConstantBuffer(uint size);
	ID3D11SamplerState* CreateSampler(TextureAddressMode mode = TEX_ADR_WRAP);

	bool IsVsyncEnabled() const { return vsync; }
	ID3D11Device* GetDevice() const { return device; }
	ID3D11DeviceContext* GetDeviceContext() const { return device_context; }
	const string& GetShadersDir() const { return shaders_dir; }

	void SetAlphaBlend(bool use_alphablend);
	void SetDepthState(DepthState depth_state);
	void SetVsync(bool vsync) { this->vsync = vsync; }
	void SetShadersDir(cstring dir) { shaders_dir = dir; }

private:
	void CreateAdapter();
	void CreateDeviceAndSwapChain();
	void CreateSizeDependentResources();
	void CreateRenderTarget();
	void CreateDepthStencilView();
	void SetViewport();
	void CreateBlendStates();
	void CreateDepthStates();

	IDXGIFactory* factory;
	IDXGIAdapter* adapter;
	IDXGISwapChain* swap_chain;
	ID3D11Device* device;
	ID3D11DeviceContext* device_context;
	ID3D11RenderTargetView* render_target;
	ID3D11DepthStencilView* depth_stencil_view;
	ID3D11BlendState* blend_state[2];
	ID3D11DepthStencilState* depth_state[DEPTH_MAX];
	DepthState current_depth_state;
	string shaders_dir;
	Int2 wnd_size;
	bool initialized, vsync, alphablend;
};
