#include "Pbr.hlsli"
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

cbuffer PER_MATERIAL : register(b2)
{
	float3 g_albedo;
	float pad;
}

Texture2D<float3> AlbedoTex : register(t0);
sampler MaterialTextureSampler : register(s1);

float4 main(VS_OUTPUT IN) : SV_TARGET
{
	float3 albedo = pow(AlbedoTex.Sample(MaterialTextureSampler, IN.texCoord), 2.2f) * g_albedo;
	albedo = FilmicToneMappingToSRGB(albedo);
	return float4(albedo, 1.0);
}
