cbuffer vs_globals : register(b0)
{
	matrix mat_combined;
	matrix mat_world;
	matrix mat_bones[32];
};

cbuffer ps_globals : register(b0)
{
	float4 ambient_color;
	float4 light_color;
	float3 light_dir;
	float4 fog_color;
	float4 fog_params;
};

Texture2D tex_diffuse;
SamplerState sampler_diffuse;

struct VS_INPUT
{
    float3 pos : POSITION;
#ifdef HAVE_WEIGHT
	half weight : BLENDWEIGHT0;
	uint4 indices : BLENDINDICES0;
#endif
	float3 normal : NORMAL;
	float2 tex : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : TEXCOORD1;
#ifdef FOG
	float pos_view_z : TEXCOORD2;
#endif
};

VS_OUTPUT vs_main(VS_INPUT In)
{
	VS_OUTPUT Out;
	
	// pos
#ifdef ANIMATED
	float3 pos = mul(float4(In.pos,1), mat_bones[In.indices[0]]).xyz * In.weight;
	pos += mul(float4(In.pos,1), mat_bones[In.indices[1]]).xyz * (1-In.weight);
	Out.pos = mul(float4(pos,1), mat_combined);
#else
	Out.pos = mul(float4(In.pos,1), mat_combined);
#endif

	// normal
#ifdef ANIMATED
	float3 normal = mul(float4(In.normal,1), mat_bones[In.indices[0]]).xyz * In.weight;
	normal += mul(float4(In.normal,1), mat_bones[In.indices[1]]).xyz * (1-In.weight);
	Out.normal = mul(float4(normal,1), mat_world).xyz;
#else
	Out.normal = mul(float4(In.normal,1), mat_world).xyz;
#endif
	
	// tex
	Out.tex = In.tex;
	
	// distance for fog
#ifdef FOG
	Out.pos_view_z = Out.pos.w;
#endif
	
	return Out;
}

float4 ps_main(VS_OUTPUT In) : SV_TARGET
{
	float4 tex = tex_diffuse.Sample(sampler_diffuse, In.tex);
	
	float3 color = saturate(ambient_color.xyz + light_color.xyz * saturate(dot(light_dir, In.normal)));
	tex.xyz = tex.xyz * color;
	
#ifdef FOG
	float fog = saturate((In.pos_view_z - fog_params.x) / fog_params.z);
	return float4(lerp(tex.xyz, fog_color.xyz, fog), tex.w);
#else
	return tex;
#endif
}
