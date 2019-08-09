float4x4 h_mat_combined;
float4 h_color;

//=============================================================================
struct Vertex
{
  float3 pos : POSITION;
};

struct VertexOutput
{
	float4 pos : POSITION;
};

void vs_simple(in Vertex In, out VertexOutput Out)
{
	Out.pos = mul(float4(In.pos,1), h_mat_combined);
}

float4 ps_simple(in VertexOutput In) : COLOR0
{
	return h_color;
}

technique tech_simple
{
	pass pass0
	{
		VertexShader = compile VS_VERSION vs_simple();
		PixelShader = compile PS_VERSION ps_simple();
	}
}

//=============================================================================
struct ColorVertex
{
  float3 pos : POSITION;
  float4 color : COLOR0;
};

struct ColorVertexOutput
{
	float4 pos : POSITION;
	float4 color : COLOR0;
};

void vs_color(in ColorVertex In, out ColorVertexOutput Out)
{
	Out.pos = mul(float4(In.pos,1), h_mat_combined);
	Out.color = In.color;
}

float4 ps_color(in ColorVertexOutput In) : COLOR0
{
	return In.color;
}

technique tech_color
{
	pass pass0
	{
		VertexShader = compile VS_VERSION vs_color();
		PixelShader = compile PS_VERSION ps_color();
	}
}
