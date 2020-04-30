#pragma pack_matrix( row_major )
#include "Pbr.hlsli"

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
	float padding;
	uint g_numPointLights;
	uint g_numQuadLights;
	float2 padding2;
	PointLight g_pointLights[8];
	QuadLight g_quadLights[8];
}

cbuffer PER_MATERIAL : register(b2)
{
	float3 albedo;
	float metallic;
	float roughness;
	float ao;
}

TextureCube irradianceMap : register(t5);
TextureCube prefilteredEnvMap : register(t6);
Texture2D BRDFIntegrationMap : register(t7);
Texture2D LTCInverseMatrixMap : register(t8);
Texture2D LTCNormMap : register(t9);
sampler SurfaceSampler : register(s0);

float4 main(VS_OUTPUT IN) : SV_TARGET
{
	float3 N = normalize(IN.normal.xyz);

	float3 color = PbrShading(
					irradianceMap,
					prefilteredEnvMap,
					BRDFIntegrationMap,
					LTCInverseMatrixMap,
					LTCNormMap,
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
	color = color / (color + float3(1.0f, 1.0f, 1.0f));
	color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));

	return float4(color, 1.0);
}
