#if defined(ANIMATED) && !defined(HAVE_WEIGHT)
#	error "Animation require weights!"
#endif

#if defined(POINT_LIGHT) && defined(DIR_LIGHT)
#	error "Mixed lighting not supported yet!"
#endif

#if defined(NORMAL_MAP) && !defined(HAVE_BINORMALS)
#	error "Normal mapping require binormals!"
#endif

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

Texture2D tex_diffuse : register(t0);
Texture2D tex_normal : register(t1);
SamplerState sampler_diffuse;
SamplerState sampler_normal;

struct VS_INPUT
{
    float3 pos : POSITION;
#ifdef HAVE_WEIGHT
	half weight : BLENDWEIGHT0;
	uint4 indices : BLENDINDICES0;
#endif
	float3 normal : NORMAL;
	float2 tex : TEXCOORD0;
#ifdef HAVE_BINORMALS
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
#endif
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
#ifdef NORMAL_MAP
	float3 tangent : TEXCOORD5;
	float3 binormal : TEXCOORD6;
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

	// tangent/binormal
#ifdef NORMAL_MAP
	Out.tangent = normalize(mul(float4(In.tangent,1), mat_world).xyz);
	Out.binormal = normalize(mul(float4(In.binormal,1), mat_world).xyz);
#endif
	
	// tex
	Out.tex = In.tex;
	
	// direction from camera to vertex for specular calculations
	Out.view_dir = normalize(camera_pos - mul(float4(pos,1), mat_world).xyz);
	
	// pos to world
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
	
#ifdef NORMAL_MAP
	float3 bump = tex_normal.Sample(sampler_normal, In.tex).xyz * 2.f - 1.f;
	float3 normal = normalize(bump.x * In.tangent + (-bump.y) * In.binormal + bump.z * In.normal);
#else
	float3 normal = In.normal;
#endif
	
#ifdef DIR_LIGHT
	float specular = 0;
	float light_intensity = saturate(dot(normal, light_dir));
	if(light_intensity > 0.f)
	{
		color = saturate(color + (light_color * light_intensity));
		
		float3 reflection = normalize(2 * light_intensity * normal - light_dir);
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
		float light_intensity = clamp(dot(light_vec, normal),0,1) * falloff;
		if(light_intensity > 0)
		{
			color.xyz += light_intensity * lights[i].color;
			float3 reflection = normalize(2 * light_intensity * normal - light_vec);
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
