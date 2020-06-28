#include "Pbr.hlsli"
#include "Light.hlsli"
#include "ColorConversion.hlsli"

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float4 posWS : TEXCOORD1;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float4 binormal : BINORMAL;
	float2 texCoord : TEXCOORD0;
};

// assume one view only for now
cbuffer PER_VIEW : register(b1)
{
	float3 g_eyePosWS;
	float padding;
	uint g_numPointLights;
	uint g_numQuadLights;
	float2 padding2;
	PointLight g_pointLights[8];
	QuadLight g_quadLights[8];
}

cbuffer PER_MATERIAL : register(b2)
{
	float3 g_albedo;
	float g_metallic;
	float g_roughness;
	float g_ao;
}

Texture2D<float3> AlbedoTex : register(t0);
Texture2D<float3> NormalTex : register(t1);
Texture2D<float3> MetallicTex : register(t2);
Texture2D<float3> RoughnessTex : register(t3);
Texture2D<float3> AOTex : register(t4);
TextureCube irradianceMap : register(t5);
TextureCube prefilteredEnvMap : register(t6);
Texture2D BRDFIntegrationMap : register(t7);
Texture2D LTCInverseMatrixMap : register(t8);
Texture2D LTCNormMap : register(t9);
Texture2D AreaLightTexture : register(t10); 
sampler SurfaceSampler : register(s0);
sampler MaterialTextureSampler : register(s1);

float4 main(VS_OUTPUT IN) : SV_TARGET
{
	float3 albedo = AlbedoTex.Sample(MaterialTextureSampler, IN.texCoord) * g_albedo;
	float metallic = MetallicTex.Sample(MaterialTextureSampler, IN.texCoord).b * g_metallic;
	float roughness = RoughnessTex.Sample(MaterialTextureSampler, IN.texCoord).g * g_roughness;
	float ao = AOTex.Sample(MaterialTextureSampler, IN.texCoord).r * g_ao;

#if NO_NORMAL_MAP
	float3 N = normalize(IN.normal.xyz);
#else 
	float3x3 TBN =
	{
		IN.tangent.x, IN.tangent.y, IN.tangent.z,
		IN.binormal.x, IN.binormal.y, IN.binormal.z,
		IN.normal.x, IN.normal.y, IN.normal.z
	};
	float3 normal = NormalTex.Sample(MaterialTextureSampler, IN.texCoord) * 2.0f - 1.0f;
	float3 N = normalize(float4(mul(normal, TBN), 0)).xyz;
#endif

	float3 color = PbrShading(
		irradianceMap,
		prefilteredEnvMap,
		BRDFIntegrationMap,
		LTCInverseMatrixMap,
		LTCNormMap,
		AreaLightTexture,
		SurfaceSampler,
		g_numPointLights,
		g_numQuadLights,
		g_pointLights,
		g_quadLights,
		IN.posWS.xyz,
		g_eyePosWS,
		N,
		albedo,
		metallic,
		roughness,
		ao
	);

	color = FilmicToneMapping(color);
	color = RGBtoSRGB(color);

	return float4(color, 1.0f);
}
