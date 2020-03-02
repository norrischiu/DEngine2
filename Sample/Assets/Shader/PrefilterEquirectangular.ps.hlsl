#pragma pack_matrix( row_major )

#include "Pbr.hlsli"

TextureCube inputTex : register(t0);
SamplerState inputSampler : register(s0);
cbuffer PrefilterCBuffer : register(b1)
{
    float roughness;
};

struct VS_OUTPUT
{
	float4 vPos : SV_POSITION;
	float3 direction : TEXCOORD0;
};

float4 main(VS_OUTPUT IN) : SV_TARGET
{
	float3 normal = normalize(IN.direction);
    float3 reflection = normal;
    float3 view = normal;

	const uint numSample = 1024;
    float weight = 0.0f;
	float3 color;

	for (uint i = 0; i < numSample; ++i)
	{
		float2 random = Hammersley(i, numSample);
		float3 H = ImportanceSampleGGX(random, normal, roughness);
		float3 L = normalize(2.0 * dot(view, H) * H - view);

		float NdotL = max(dot(normal, L), 0.0);
        if(NdotL > 0.0)
        {
            color += inputTex.Sample(inputSampler, L).rgb * NdotL;
			weight += NdotL;
        }
	}

	return float4(color / weight, 1.0f);
}
