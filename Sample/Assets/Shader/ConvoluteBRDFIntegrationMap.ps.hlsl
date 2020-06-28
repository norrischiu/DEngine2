#pragma pack_matrix( row_major )

#define IBL
#include "Pbr.hlsli"

cbuffer SizeCBuffer : register(b1)
{
	float2 textureSize;
}

struct VS_OUTPUT
{
	float4 vPos : SV_POSITION;
};

float2 main(VS_OUTPUT IN) : SV_TARGET
{
	float NdotV = IN.vPos.x / textureSize.x;
	float roughness = IN.vPos.y / textureSize.y;

	float3 normal = float3(0.0f, 1.0f, 0.0f);
	float3 view = float3(sqrt(1.0f - NdotV * NdotV), NdotV, 0.0f);

	const uint numSample = 1024;
	float scale, bias;

	for (uint i = 0; i < numSample; ++i)
	{
		float2 random = RandomFloat2_Hammersley(i, numSample);
		float3 halfVector = ImportanceSample_InverseCDF_GGX(random, normal, roughness);
		float3 light = normalize(2.0f * dot(view, halfVector) * halfVector - view);

        float NdotL = saturate(light.y);
        float NdotH = saturate(halfVector.y);
        float VdotH = saturate(dot(view, halfVector));

        if(NdotL > 0.0)
        {
            float G = Geometry_Smith(normal, view, light, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float oneMinusVdotH_power5 = pow(1.0 - VdotH, 5.0f);

            scale += (1.0f - oneMinusVdotH_power5) * G_Vis;
            bias += oneMinusVdotH_power5 * G_Vis;
        }
	}

	return float2(scale / numSample, bias / numSample);
}