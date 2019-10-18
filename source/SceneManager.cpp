#include "EnginePch.h"
#include "EngineCore.h"
#include "SceneManager.h"
#include "Scene.h"
#include "SceneNode.h"
#include "Camera.h"
#include "Mesh.h"
#include "Render.h"
#include "DirectX.h"

SceneManager* app::scene_mgr;

SceneManager::SceneManager() : scene(nullptr), camera(nullptr)
{
}

SceneManager::~SceneManager()
{
	DeleteElements(scenes);
	DeleteElements(cameras);

	SafeRelease(vertex_shader);
	SafeRelease(pixel_shader);
	SafeRelease(layout);
	SafeRelease(vs_buffer);
	SafeRelease(sampler);
}

void SceneManager::Init()
{
	ID3DBlob* vs_buf = app::render->CompileShader("test.hlsl", "vs_main", true);
	HRESULT result = app::render->device->CreateVertexShader(vs_buf->GetBufferPointer(), vs_buf->GetBufferSize(), nullptr, &vertex_shader);
	//if(FAILED(result))
	//	throw Format("Failed to create vertex shader '%s' (%u).", filename, result);

	ID3DBlob* ps_buf = app::render->CompileShader("test.hlsl", "ps_main", false);
	result = app::render->device->CreatePixelShader(ps_buf->GetBufferPointer(), ps_buf->GetBufferSize(), nullptr, &pixel_shader);
	//if(FAILED(result))
	//	throw Format("Failed to create pixel shader '%s' (%u).", filename, result);

	// create layout
	D3D11_INPUT_ELEMENT_DESC desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	result = app::render->device->CreateInputLayout(desc, countof(desc), vs_buf->GetBufferPointer(), vs_buf->GetBufferSize(), &layout);
	//if(FAILED(result))
	//	throw Format("Failed to create input layout '%s' (%u).", filename, result);

	vs_buffer = app::render->CreateConstantBuffer(sizeof(Matrix));

	vs_buf->Release();
	ps_buf->Release();

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

	result = app::render->device->CreateSamplerState(&sampler_desc, &sampler);
	if(FAILED(result))
		throw Format("Failed to create sampler state (%u).", result);
}

void SceneManager::Draw()
{
	ID3D11DeviceContext* device_context = app::render->device_context;

	Vec4 color = scene ? scene->clear_color : Color::Black;
	device_context->ClearRenderTargetView(app::render->render_target, (float*)color);
	device_context->ClearDepthStencilView(app::render->depth_stencil_view, D3D11_CLEAR_DEPTH, 1.f, 0);

	if(!scene || !camera)
	{
		V(app::render->swap_chain->Present(app::render->vsync ? 1 : 0, 0));
		return;
	}

	device_context->IASetInputLayout(layout);
	device_context->VSSetShader(vertex_shader, nullptr, 0);
	device_context->PSSetShader(pixel_shader, nullptr, 0);
	device_context->PSSetSamplers(0, 1, &sampler);

	Matrix mat_view = Matrix::CreateLookAt(camera->from, camera->to, camera->up),
		mat_proj = Matrix::CreatePerspectiveFieldOfView(PI / 4, 1024.f / 768, 0.1f, 50.f),
		mat_view_proj = mat_view * mat_proj;

	for(SceneNode* node : scene->nodes)
	{
		Matrix mat_world = Matrix::Rotation(node->rot) * Matrix::Translation(node->pos),
			mat_combined = mat_world * mat_view_proj;

		// set vs consts
		D3D11_MAPPED_SUBRESOURCE resource;
		V(device_context->Map(vs_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
		Matrix& g = *(Matrix*)resource.pData;
		g = mat_combined.Transpose();
		device_context->Unmap(vs_buffer, 0);
		device_context->VSSetConstantBuffers(0, 1, &vs_buffer);

		Mesh* mesh = node->mesh;
		uint stride = mesh->vertex_size,
			offset = 0;
		device_context->IASetVertexBuffers(0, 1, &mesh->vb, &stride, &offset);
		device_context->IASetIndexBuffer(mesh->ib, DXGI_FORMAT_R16_UINT, 0);

		for(Mesh::Submesh& sub : mesh->subs)
		{
			device_context->PSSetShaderResources(0, 1, &sub.tex->view);
			device_context->DrawIndexed(sub.tris * 3, sub.first * 3, sub.min_ind);
		}
	}

	V(app::render->swap_chain->Present(app::render->vsync ? 1 : 0, 0));
}
