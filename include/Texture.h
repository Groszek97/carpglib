#pragma once

//-----------------------------------------------------------------------------
#include "Resource.h"

//-----------------------------------------------------------------------------
struct Texture : public Resource
{
	static constexpr ResourceType Type = ResourceType::Texture;

	TEX tex;

	Texture() : tex(nullptr) {}
	~Texture();
	void ResizeImage(Int2& new_size, Int2& img_size, Vec2& scale);
	Int2 GetSize() const;
};

//-----------------------------------------------------------------------------
struct TexOverride
{
	explicit TexOverride(Texture* diffuse = nullptr) : diffuse(diffuse), normal(nullptr), specular(nullptr) {}
	int GetIndex() const
	{
		return (normal ? 2 : 0) + (specular ? 1 : 0);
	}

	TexturePtr diffuse, normal, specular;
};

//-----------------------------------------------------------------------------
struct TextureLock
{
	TextureLock(TEX tex);
	~TextureLock();
	uint* operator [] (uint row) { return (uint*)(data + pitch * row); }
	void GenerateMipSubLevels();

private:
	TEX tex;
	ID3D11Texture2D* res;
	byte* data;
	int pitch;
};
