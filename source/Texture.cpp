#include "EnginePch.h"
#include "EngineCore.h"
#include "Texture.h"
#include "Render.h"
#include "DirectX.h"

//=================================================================================================
Texture::~Texture()
{
	SafeRelease(tex);
}

//=================================================================================================
void Texture::ResizeImage(Int2& new_size, Int2& img_size, Vec2& scale)
{
	img_size = GetSize();
	if(new_size == Int2(0, 0))
	{
		new_size = img_size;
		scale = Vec2(1, 1);
	}
	else if(new_size == img_size)
		scale = Vec2(1, 1);
	else
		scale = Vec2(float(new_size.x) / img_size.x, float(new_size.y) / img_size.y);
}

//=================================================================================================
Int2 Texture::GetSize() const
{
	ID3D11Texture2D* res;
	tex->GetResource(reinterpret_cast<ID3D11Resource**>(&res));
	D3D11_TEXTURE2D_DESC desc;
	res->GetDesc(&desc);
	Int2 size(desc.Width, desc.Height);
	res->Release();
	return size;
}

//=================================================================================================
TextureLock::TextureLock(TEX tex) : tex(tex)
{
	assert(tex);

	ID3D11Texture2D* res;
	tex->GetResource(reinterpret_cast<ID3D11Resource**>(&res));

	D3D11_MAPPED_SUBRESOURCE resource;
	app::render->GetDeviceContext()->Map(res, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	data = static_cast<byte*>(resource.pData);
	pitch = resource.RowPitch;
}

//=================================================================================================
TextureLock::~TextureLock()
{
	if(res)
	{
		app::render->GetDeviceContext()->Unmap(res, 0);
		res->Release();
	}
}

//=================================================================================================
void TextureLock::GenerateMipSubLevels()
{
	assert(res);
	ID3D11DeviceContext* device_context = app::render->GetDeviceContext();
	device_context->Unmap(res, 0);
	device_context->GenerateMips(tex);
	res->Release();
	res = nullptr;
}
