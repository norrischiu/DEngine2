#include "Pbr.hlsli"
#include "Light.hlsli"

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float4 posWS : TEXCOORD1;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float4 binormal : BINORMAL;
	float2 texCorod : TEXCOORD0;
};

// assume one view only for now
cbuffer PER_VIEW : register(b1)
{
	float3 g_eyePosWS;
	uint g_numPointLights;
	Light g_pointLights[8];
}

Texture2D<float3> AlbedoTex : register(t0);
Texture2D<float3> NormalTex : register(t1);
Texture2D<float3> MetallicTex : register(t2);
Texture2D<float3> RoughnessTex : register(t3);
Texture2D<float3> AOTex : register(t4);
TextureCube irradianceMap : register(t5);
TextureCube prefilteredEnvMap : register(t6);
Texture2D BRDFIntegrationMap : register(t7);
sampler SurfaceSampler : register(s0);
sampler MaterialTextureSampler : register(s1);

float4 main(VS_OUTPUT IN) : SV_TARGET
{
	float3 albedo = pow(AlbedoTex.Sample(MaterialTextureSampler, IN.texCorod), 2.2f);
	float3 normal = NormalTex.Sample(MaterialTextureSampler, IN.texCorod) * 2.0f - 1.0f;
	float metallic = MetallicTex.Sample(MaterialTextureSampler, IN.texCorod).b;
	float roughness = RoughnessTex.Sample(MaterialTextureSampler, IN.texCorod).g;
	float ao = AOTex.Sample(MaterialTextureSampler, IN.texCorod).r;

	float3x3 TBN =
	{
		IN.tangent.x, IN.tangent.y, IN.tangent.z,
		IN.binormal.x, IN.binormal.y, IN.binormal.z,
		IN.normal.x, IN.normal.y, IN.normal.z
	};
	float3 N = normalize(float4(mul(normal, TBN), 0)).xyz;

	float3 color = PbrShading(
		irradianceMap,
		prefilteredEnvMap,
		BRDFIntegrationMap,
		SurfaceSampler,
		g_numPointLights,
		g_pointLights,
		IN.posWS.xyz,
		g_eyePosWS,
		N,
		albedo,
		metallic,
		roughness,
		ao
	);
	color = color / (color + float3(1.0f, 1.0f, 1.0f));
	color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2)); 

	return float4(color, 1.0f);
}
