#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"
#include "VertexDeclaration.h"

//-----------------------------------------------------------------------------
class DebugDrawer : public ShaderHandler
{
public:
	typedef delegate<void(DebugDrawer*)> Handler;

	enum class Shape
	{
		Box,
		Capsule,
		Max
	};

	DebugDrawer();
	~DebugDrawer();
	void OnInit() override;
	void OnReset() override;
	void OnReload() override;
	void OnRelease() override;
	void Begin(Camera& camera);
	void End();
	void BeginBatch();
	void EndBatch();
	void DrawShape(Shape shape, const Matrix& m);
	void AddQuad(const Vec3(&pts)[4]);

	Handler GetHandler() const { return handler; }

	void SetHandler(Handler handler) { this->handler = handler; }
	void SetColor(Color color) { this->color = color; }

private:
	Render* render;
	IDirect3DDevice9* device;
	Handler handler;
	ID3DXEffect* effect;
	D3DXHANDLE tech_simple, tech_color;
	D3DXHANDLE h_mat_combined, h_color;
	Mesh* shapes[(int)Shape::Max];
	IDirect3DVertexDeclaration9* vertex_decl;
	VB vb;
	uint vb_size;
	Vec4 color;
	vector<VColor> verts;
	Matrix mat_view_proj;
	bool batch;
};
