cbuffer vs_globals : register(b0)
{
	matrix mat_combined;
};
Texture2D texture0;
SamplerState sampler0;

struct VS_INPUT
{
    float3 pos : POSITION;
	float2 tex : TEXCOORD0;
	float4 color : COLOR0;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
	float4 color : COLOR0;
};

VS_OUTPUT vs_main(VS_INPUT In)
{
	VS_OUTPUT Out;
	Out.pos = mul(float4(In.pos,1), mat_combined);
	Out.tex = In.tex;
	Out.color = In.color;
	return Out;
}

float4 ps_main(VS_OUTPUT In) : SV_TARGET
{
	float4 tex = texture0.Sample(sampler0, In.tex);
	return lerp(tex, In.color, 0.5);
}
