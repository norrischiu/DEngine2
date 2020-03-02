#pragma pack_matrix( row_major )

#define IBL
#include "Pbr.hlsli"

TextureCube inputTex : register(t0);
SamplerState inputSampler : register(s0);
cbuffer SizeCBuffer : register(b1)
{
	float2 textureSize;
}

struct VS_OUTPUT
{
	float4 vPos : SV_POSITION;
};

float4 main(VS_OUTPUT IN) : SV_TARGET
{
	float NdotV = IN.vPos.x / textureSize.x;
	float roughness = IN.vPos.y / textureSize.y;

	float3 normal = float3(0.0f, 1.0f, 0.0f);
    float3 view = normal;

	const uint numSample = 1024;
	float scale, bias;

	for (uint i = 0; i < numSample; ++i)
	{
		float2 random = Hammersley(i, numSample);
		float3 halfVector = ImportanceSampleGGX(random, normal, roughness);
		float3 light = normalize(2.0f * dot(view, halfVector) * halfVector - view);

        float NdotL = max(light.y, 0.0f);
        float NdotH = max(halfVector.y, 0.0f);
        float VdotH = max(dot(view, halfVector), 0.0f);

        if(NdotL > 0.0)
        {
            float G = GeometrySmith(normal, view, light, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float fresnel0 = pow(1.0 - VdotH, 5.0f);

            scale += (1.0f - fresnel0) * G_Vis;
            bias += fresnel0 * G_Vis;
        }
	}

	return float4(scale, bias, 0.0f, 1.0f);
}