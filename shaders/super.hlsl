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

struct Light
{
	float3 color;
	float3 pos;
	float range;
};

cbuffer ps_locals : register(b1)
{
	float4 tint;
	Light lights[3];
};

cbuffer ps_material : register(b2)
{
	float3 specular_color;
	float specular_hardness;
	float specular_intensity;
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
	float3 view_dir : TEXCOORD2;
#ifdef POINT_LIGHT
	float3 pos_world : TEXCOORD3;
#endif
#ifdef FOG
	float pos_view_z : TEXCOORD4;
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
	
#ifdef POINT_LIGHT
	Out.pos_world = mul(float4(pos,1), mat_world).xyz;
#endif
	
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
	
#ifdef DIR_LIGHT
	float specular = 0;
	float light_intensity = saturate(dot(In.normal, light_dir));
	if(light_intensity > 0.f)
	{
		color = saturate(color + (light_color * light_intensity));
		
		float3 reflection = normalize(2 * light_intensity * In.normal - light_dir);
		specular = pow(saturate(dot(reflection, In.view_dir)), specular_hardness) * specular_intensity;
	}
	tex.xyz = saturate((tex.xyz * color.xyz) + specular_color * specular);
#elif defined(POINT_LIGHT)
	float specular = 0;
	for(int i=0; i<3; ++i)
	{
		float3 light_vec = normalize(lights[i].pos - In.pos_world);
		float dist = distance(lights[i].pos, In.pos_world);
		float falloff = clamp((1 - (dist / lights[i].range)), 0, 1);
		float light_intensity = clamp(dot(light_vec, In.normal),0,1) * falloff;
		if(light_intensity > 0)
		{
			color.xyz += light_intensity * lights[i].color;
			float3 reflection = normalize(2 * light_intensity * In.normal - light_vec);
			specular += pow(saturate(dot(reflection, normalize(In.view_dir))), specular_hardness) * specular_intensity * falloff;
		}
	}
	specular = saturate(specular);
	tex.xyz = saturate((tex.xyz * saturate(color.xyz)) + specular_color * specular);
#endif
	
#ifdef FOG
	float fog = saturate((In.pos_view_z - fog_params.x) / fog_params.z);
	return float4(lerp(tex.xyz, fog_color.xyz, fog), tex.w);
#else
	return tex;
#endif
}
