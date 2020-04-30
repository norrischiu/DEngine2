#include "Pbr.hlsli"

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
};

cbuffer PER_MATERIAL : register(b2)
{
	float3 albedo;
	float metallic;
	float roughness;
	float ao;
}

float4 main(VS_OUTPUT IN) : SV_TARGET
{
	float3 color = albedo;
	color = color / (color + float3(1.0f, 1.0f, 1.0f));
	color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));

	return float4(color, 1.0);
}
