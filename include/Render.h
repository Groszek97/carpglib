#pragma once

//-----------------------------------------------------------------------------
#include "VertexDeclaration.h"

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
	D3DXMACRO* macros;
	ID3DXEffectPool* pool;
};

//-----------------------------------------------------------------------------
class Render
{
public:
	Render();
	~Render();
	void Init();
	bool Reset(bool force);
	void WaitReset();
	bool CanDraw();
	void Present();
	bool CheckDisplay(const Int2& size, int& hz); // dla zera zwraca najlepszy hz
	void RegisterShader(ShaderHandler* shader);
	ID3DXEffect* CompileShader(cstring name);
	ID3DXEffect* CompileShader(CompileShaderParams& params);
	void ReloadShaders();
	TEX CreateTexture(const Int2& size, Color* fill = nullptr);
	RenderTarget* CreateRenderTarget(const Int2& size);
	Texture* CopyToTexture(RenderTarget* target);
	bool IsLostDevice() const { return lost_device; }
	bool IsMultisamplingEnabled() const { return multisampling != 0; }
	bool IsVsyncEnabled() const { return vsync; }
	IDirect3DDevice9* GetDevice() const { return device; }
	ID3DXSprite* GetSprite() const { return sprite; }
	void GetMultisampling(int& ms, int& msq) const { ms = multisampling; msq = multisampling_quality; }
	void GetResolutions(vector<Resolution>& v) const;
	void GetMultisamplingModes(vector<Int2>& v) const;
	int GetRefreshRate() const { return refresh_hz; }
	vector<ShaderHandler*>& GetShaders() { return shaders; }
	int GetShaderVersion() const { return shader_version; }
	int GetAdapter() const { return used_adapter; }
	const string& GetShadersDir() const { return shaders_dir; }
	IDirect3DVertexDeclaration9* GetVertexDeclaration(VertexDeclarationId id) { return vertex_decl[id]; }
	void SetAlphaBlend(bool alphablend);
	void SetAlphaTest(bool alphatest);
	void SetNoCulling(bool nocull);
	void SetNoZWrite(bool nozwrite);
	void SetWireframe(bool wireframe);
	void SetVsync(bool vsync);
	int SetMultisampling(int type, int quality);
	void SetRefreshRateInternal(int refresh_hz) { this->refresh_hz = refresh_hz; }
	void SetShaderVersion(int shader_version) { this->shader_version = shader_version; }
	void SetTarget(RenderTarget* target);
	void SetTextureAddressMode(TextureAddressMode mode);
	void SetShadersDir(cstring dir) { shaders_dir = dir; }
	void SetAdapter(int adapter) { assert(!initialized); used_adapter = adapter; }

private:
	void GatherParams(D3DPRESENT_PARAMETERS& d3dpp);
	void LogMultisampling();
	void LogAndSelectResolution();
	void SetDefaultRenderState();
	void CreateVertexDeclarations();
	void CreateRenderTargetTexture(RenderTarget* target);
	void BeforeReset();
	void AfterReset();

	IDirect3D9* d3d;
	IDirect3DDevice9* device;
	ID3DXSprite* sprite;
	vector<ShaderHandler*> shaders;
	vector<RenderTarget*> targets;
	IDirect3DVertexDeclaration9* vertex_decl[VDI_MAX];
	RenderTarget* current_target;
	SURFACE current_surf;
	string shaders_dir;
	int used_adapter, shader_version, refresh_hz, multisampling, multisampling_quality;
	bool initialized, vsync, lost_device, res_freed, alphatest, nozwrite, nocull, alphablend, wireframe;
};
