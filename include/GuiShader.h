#pragma once

//-----------------------------------------------------------------------------
#include "VertexDeclaration.h"

//-----------------------------------------------------------------------------
class GuiShader
{
public:
	GuiShader();
	~GuiShader();
	void Init();
	void Prepare();
	void SetGrayscale(bool grayscale);
	void Draw(TEX tex, VParticle* verts, uint quads);

private:
	ID3D11DeviceContext* device_context;
	ID3D11VertexShader* vertex_shader;
	ID3D11PixelShader* pixel_shader;
	ID3D11InputLayout* layout;
	ID3D11Buffer* vs_buffer;
	ID3D11Buffer* ps_buffer;
	ID3D11SamplerState* sampler;
	ID3D11Buffer* vb;
	TEX current_tex;
	TEX empty_tex;
};
