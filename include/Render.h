#pragma once

//-----------------------------------------------------------------------------
enum TextureAddressMode
{
	TEX_ADR_WRAP = 1,
	TEX_ADR_MIRROR = 2,
	TEX_ADR_CLAMP = 3,
	TEX_ADR_BORDER = 4,
	TEX_ADR_MIRRORONCE = 5
};

//-----------------------------------------------------------------------------
struct Resolution
{
	Int2 size;
	uint hz;
};

//-----------------------------------------------------------------------------
//struct CompileShaderParams
//{
//	cstring name;
//	cstring cache_name;
//	string* input;
//	FileTime file_time;
//	//D3DXMACRO* macros;
//	//ID3DXEffectPool* pool;
//};
FIXME;

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

	Render();
	~Render();
	void Init();
	//ID3DXEffect* CompileShader(cstring name);
	//ID3DXEffect* CompileShader(CompileShaderParams& params);
	TEX CreateTexture(const Int2& size, Color* fill);
	//RenderTarget* CreateRenderTarget(const Int2& size);
	//Texture* CopyToTexture(RenderTarget* target);

	bool IsMultisamplingEnabled() const { return multisampling != 0; }
	bool IsVsyncEnabled() const { return vsync; }

	ID3D11Device* GetDevice() const { return device; }
	ID3D11DeviceContext* GetDeviceContext() const { return device_context; }
	void GetMultisampling(int& ms, int& msq) const { ms = multisampling; msq = multisampling_quality; }
	void GetResolutions(vector<Resolution>& v) const;
	void GetMultisamplingModes(vector<Int2>& v) const;
	const string& GetShadersDir() const { return shaders_dir; }
	//IDirect3DVertexDeclaration9* GetVertexDeclaration(VertexDeclarationId id) { return vertex_decl[id]; }

	void SetAlphaBlend(bool use_alphablend);
	void SetAlphaTest(bool use_alphatest);
	void SetNoCulling(bool use_nocull);
	void SetDepthState(DepthState depth_state);
	void SetVsync(bool vsync) { this->vsync = vsync; }
	int SetMultisampling(int type, int quality);
	void SetTarget(RenderTarget* target);
	void SetShadersDir(cstring dir) { shaders_dir = dir; }

	void OnChangeResolution();
	void CreateShader(cstring filename, D3D11_INPUT_ELEMENT_DESC* input, uint input_count, ID3D11VertexShader*& vertex_shader,
		ID3D11PixelShader*& pixel_shader, ID3D11InputLayout*& layout);
	ID3DBlob* CompileShader(cstring filename, cstring entry, bool is_vertex);
	ID3D11Buffer* CreateConstantBuffer(uint size);
	ID3D11SamplerState* CreateSampler(TextureAddressMode mode = TEX_ADR_WRAP);

private:
	void CreateAdapter();
	void CreateDeviceAndSwapChain();
	void CreateSizeDependentResources();
	void CreateRenderTarget();
	void CreateDepthStencilView();
	void SetViewport();
	void CreateBlendStates();
	void CreateDepthStates();

	//
	void LogMultisampling();
	void LogAndSelectResolution();
	void CreateRenderTargetTexture(RenderTarget* target);

	IDXGIFactory* factory;
	IDXGIAdapter* adapter;
	IDXGISwapChain* swap_chain;
	ID3D11Device* device;
	ID3D11DeviceContext* device_context;
	ID3D11RenderTargetView* render_target;
	ID3D11DepthStencilView* depth_stencil_view;


	//ID3D11RasterizerState* raster_state[RASTER_MAX];
	ID3D11BlendState* blend_state[2];
	ID3D11DepthStencilState* depth_state[DEPTH_MAX];
	DepthState current_depth_state;


	vector<RenderTarget*> targets;
	//RenderTarget* current_target;
	//SURFACE current_surf;
	string shaders_dir;
	Int2 wnd_size;
	int multisampling, multisampling_quality;
	bool initialized, vsync, r_alphatest, r_nocull, alphablend;
};
FIXME;
