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

cbuffer LIGHT : register(b1)
{
	float4		g_lightPosWS;
	float4		g_eyePosWS;
};

Texture2D<float3> AlbedoTex : register(t0);
Texture2D<float3> NormalTex : register(t1);
Texture2D<float3> MetallicTex : register(t2);
Texture2D<float3> RoughnessTex : register(t3);
Texture2D<float3> AOTex : register(t4);

sampler SurfaceSampler : register(s0);

float4 main(VS_OUTPUT IN) : SV_TARGET
{
	float3 albedo = pow(AlbedoTex.Sample(SurfaceSampler, IN.texCorod), 2.2f);
	float3 normal = NormalTex.Sample(SurfaceSampler, IN.texCorod) * 2.0f - 1.0f;
	float metallic = MetallicTex.Sample(SurfaceSampler, IN.texCorod).r;
	float roughness = RoughnessTex.Sample(SurfaceSampler, IN.texCorod).r;
	float ao = AOTex.Sample(SurfaceSampler, IN.texCorod).r;

	float3x3 TBN =
	{
		IN.tangent.x, IN.tangent.y, IN.tangent.z,
		IN.binormal.x, IN.binormal.y, IN.binormal.z,
		IN.normal.x, IN.normal.y, IN.normal.z
	};
	float3 N = normalize(float4(mul(normal, TBN), 0)).xyz;
	float3 V = normalize(g_eyePosWS - IN.posWS).xyz;

	float3 F0 = float3(0.04f, 0.04f, 0.04f); // default F0 for dielectrics, not accurate but close enough
	F0 = lerp(F0, albedo, metallic);

	// reflectance equation
	float3 Lo = float3(0.0f, 0.0f, 0.0f);
	// calculate per-light radiance
	float3 L = normalize(g_lightPosWS - IN.posWS).xyz;
	float3 H = normalize(V + L);
	float distance = length(g_lightPosWS - IN.posWS);
	float attenuation = 1.0 / (distance * distance);
	float3 radiance = /*lightColors[i] * */attenuation;

	// cook-torrance brdf
	float NDF = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

	float3 kS = F;
	float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
	kD *= 1.0 - metallic;

	float3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
	float3 specular = numerator / max(denominator, 0.001);

	// add to outgoing radiance Lo
	float NdotL = max(dot(N, L), 0.0);
	Lo += (kD * albedo / PI + specular) * radiance * NdotL;

	float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo * ao;
	float3 color = ambient + Lo;

	color = color / (color + float3(1.0f, 1.0f, 1.0f));
	color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));

	return float4(color, 1.0);
}
