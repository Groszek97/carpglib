#pragma once

class FontLoader
{
public:
	FontLoader();
	Font* Load(cstring name, int size, int weight);

private:
	void InitGdi();
	void RenderFontToTexture(ID3D11Texture2D* tex, Font* font, void* winapi_font);

	ID3D11Device* device;
	const int padding = 2;
	bool gdi_initialized;
};
