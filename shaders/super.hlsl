cbuffer vs_globals : register(b0)
{
	float3 camera_pos;
};

cbuffer vs_locals : register(b1)
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

cbuffer ps_locals : register(b1)
{
	float4 tint;
	float3 specular_color;
	float specular_hardness;
	float specular_intensity;
}

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
	float3 view_dir : TEXCOORD2;
#ifdef FOG
	float pos_view_z : TEXCOORD3;
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
	float3 pos = In.pos;
	Out.pos = mul(float4(pos,1), mat_combined);
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
	
	// direction from camera to vertex for specular calculations
	Out.view_dir = normalize(camera_pos - mul(float4(pos,1), mat_world).xyz);
	
	// distance for fog
#ifdef FOG
	Out.pos_view_z = Out.pos.w;
#endif

	return Out;
}

float4 ps_main(VS_OUTPUT In) : SV_TARGET
{
	float4 tex = tex_diffuse.Sample(sampler_diffuse, In.tex) * tint;
	
	float4 color = ambient_color;
	float specular = 0;
	
	float light_intensity = saturate(dot(In.normal, light_dir));
	if(light_intensity > 0.f)
	{
		color = saturate(color + (light_color * light_intensity));
		
		float3 reflection = normalize(2 * light_intensity * In.normal - light_dir);
		specular = pow(saturate(dot(reflection, In.view_dir)), specular_hardness) * specular_intensity;
	}
	
	tex.xyz = saturate((tex.xyz * color.xyz) + specular_color * specular);
	
#ifdef FOG
	float fog = saturate((In.pos_view_z - fog_params.x) / fog_params.z);
	return float4(lerp(tex.xyz, fog_color.xyz, fog), tex.w);
#else
	return tex;
#endif
}
