#pragma once

//-----------------------------------------------------------------------------
#include "VertexDeclaration.h"
//---------------
#include "Timer.h"

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
struct CompileShaderParams
{
	cstring name;
	cstring cache_name;
	string* input;
	FileTime file_time;
	//D3DXMACRO* macros;
	//ID3DXEffectPool* pool;
	FIXME;
};

//-----------------------------------------------------------------------------
class Render
{
public:
	Render();
	~Render();
	void Init();
	void Draw(bool call_present = true);
	bool CheckDisplay(const Int2& size, int& hz); // dla zera zwraca najlepszy hz
	void RegisterShader(ShaderHandler* shader);
	//ID3DXEffect* CompileShader(cstring name);
	//ID3DXEffect* CompileShader(CompileShaderParams& params);
	//TEX CreateTexture(const Int2& size);
	//RenderTarget* CreateRenderTarget(const Int2& size);
	//Texture* CopyToTexture(RenderTarget* target);
	bool IsMultisamplingEnabled() const { return multisampling != 0; }
	bool IsVsyncEnabled() const { return vsync; }
	//IDirect3DDevice9* GetDevice() const { return device; }
	void GetMultisampling(int& ms, int& msq) const { ms = multisampling; msq = multisampling_quality; }
	void GetResolutions(vector<Resolution>& v) const;
	void GetMultisamplingModes(vector<Int2>& v) const;
	int GetRefreshRate() const { return refresh_hz; }
	vector<ShaderHandler*>& GetShaders() { return shaders; }
	int GetShaderVersion() const { return shader_version; }
	const string& GetShadersDir() const { return shaders_dir; }
	//IDirect3DVertexDeclaration9* GetVertexDeclaration(VertexDeclarationId id) { return vertex_decl[id]; }
	void SetAlphaBlend(bool use_alphablend);
	void SetAlphaTest(bool use_alphatest);
	void SetNoCulling(bool use_nocull);
	void SetNoZWrite(bool use_nozwrite);
	void SetVsync(bool vsync);
	int SetMultisampling(int type, int quality);
	void SetRefreshRateInternal(int refresh_hz) { this->refresh_hz = refresh_hz; }
	void SetShaderVersion(int shader_version) { this->shader_version = shader_version; }
	void SetTarget(RenderTarget* target);
	void SetTextureAddressMode(TextureAddressMode mode);
	void SetShadersDir(cstring dir) { shaders_dir = dir; }

private:
	void CreateAdapter();
	void CreateDeviceAndSwapChain();
	void CreateSizeDependentResources();
	void CreateRenderTarget();
	void CreateDepthStencilView();
	void SetViewport();
	void InitTmp();
	ID3DBlob* CompileShader(cstring filename, cstring entry, bool is_vertex);
	ID3D11Buffer* CreateConstantBuffer(uint size);

	//
	//void GatherParams(D3DPRESENT_PARAMETERS& d3dpp);
	void LogMultisampling();
	void LogAndSelectResolution();
	void SetDefaultRenderState();
	void CreateVertexDeclarations();
	void CreateRenderTargetTexture(RenderTarget* target);

	IDXGIFactory* factory;
	IDXGIAdapter* adapter;
	IDXGISwapChain* swap_chain;
	ID3D11Device* device;
	ID3D11DeviceContext* device_context;
	ID3D11RenderTargetView* render_target;
	ID3D11DepthStencilView* depth_stencil_view;


	ID3D11VertexShader* vertex_shader;
	ID3D11PixelShader* pixel_shader;
	ID3D11InputLayout* layout;
	ID3D11Buffer* vs_buffer;
	ID3D11Buffer* vb;

	/*ID3D11DepthStencilState* depth_state[DEPTH_MAX];
	ID3D11RasterizerState* raster_state[RASTER_MAX];
	ID3D11BlendState* blend_state[BLEND_MAX];*/


	vector<ShaderHandler*> shaders;
	vector<RenderTarget*> targets;
	//IDirect3DVertexDeclaration9* vertex_decl[VDI_MAX];
	//RenderTarget* current_target;
	//SURFACE current_surf;
	string shaders_dir;
	Int2 wnd_size;
	int shader_version, refresh_hz, multisampling, multisampling_quality;
	bool initialized, vsync, r_alphatest, r_nozwrite, r_nocull, r_alphablend;

	Timer t;
	float rot;
};
FIXME;
