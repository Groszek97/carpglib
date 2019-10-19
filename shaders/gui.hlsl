cbuffer vs_globals : register(b0)
{
	float2 size;
};
cbuffer ps_globals : register(b0)
{
	bool grayscale;
};
Texture2D texture0;
SamplerState sampler0;

struct VERTEX_INPUT
{
	float3 pos : POSITION;
	float2 tex : TEXCOORD0;
	float4 color : COLOR0;
};

struct VERTEX_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
	float4 color : COLOR0;
};

void vs_entry(in VERTEX_INPUT In, out VERTEX_OUTPUT Out)
{
	// fix half pixel problem
	Out.pos.x = ((In.pos.x - 0.5f) / (size.x * 0.5f)) - 1.0f;
	Out.pos.y = -(((In.pos.y - 0.5f) / (size.y * 0.5f)) - 1.0f);
	Out.pos.z = 0.f;
	Out.pos.w = 1.f;
	Out.tex = In.tex;
	Out.color = In.color;
}

float4 ps_entry(in VERTEX_OUTPUT In) : SV_TARGET
{
	float4 c = texture0.Sample(sampler0, In.tex) * In.color;
	//if(grayscale)
	//	c.rgb = (c.r+c.g+c.b)/3.0f;
	return c;
}
