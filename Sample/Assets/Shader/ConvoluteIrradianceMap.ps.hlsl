#include "pbr.hlsli"

#pragma pack_matrix( row_major )

TextureCube inputTex : register(t0);
SamplerState inputSampler : register(s0);

struct VS_OUTPUT
{
	float4 vPos : SV_POSITION;
	float3 direction : TEXCOORD0;
};

float4 main(VS_OUTPUT IN) : SV_TARGET
{
	float3 normal = normalize(IN.direction);
	float3 irradiance = float3(0.0f, 0.0f, 0.0f);

	const uint numSamples = 4096;
	for (uint i = 0; i < numSamples; ++i)
	{
		float2 random = RandomFloat2_Hammersley(i, numSamples);
		float3 L = ImportanceSample_DiffuseLobe(random, normal);
		float NdotL = dot(normal, L);
		if (NdotL > 0)
		{
			irradiance += clamp(inputTex.Sample(inputSampler, L).rgb, 0.0f, 16.0f);
		}
	}

	return float4(irradiance / (float)numSamples, 1.0f);
}
