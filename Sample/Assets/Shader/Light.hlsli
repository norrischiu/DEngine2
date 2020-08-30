// Light.hlsli: header file for light related struct and calculation
#ifndef LIGHT_HLSLI
#define LIGHT_HLSLI
#pragma pack_matrix( row_major )

struct PointLight
{
	float3 positionWS;
	float falloffRadius;
	float3 color;
	float intensity;
};

struct QuadLight
{
	float4 vertices[4];
	float3 color;
	float intensity;
	float falloffRadius;
	float3 centerWS;
};
#endif