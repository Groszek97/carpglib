cbuffer vs_globals : register(b0)
{
	matrix mat_combined;
};

struct VS_INPUT
{
    float3 pos : POSITION;
	float4 color : COLOR0;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
	float4 color : COLOR0;
};

VS_OUTPUT vs_main(VS_INPUT In)
{
	VS_OUTPUT Out;
	Out.pos = mul(float4(In.pos,1), mat_combined);
	Out.color = In.color;
	return Out;
}

float4 ps_main(VS_OUTPUT In) : SV_TARGET
{
	return In.color;
}
