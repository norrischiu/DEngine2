// Light.hlsli: header file for light related struct and calculation
#ifndef LIGHT_HLSLI
#define LIGHT_HLSLI

struct Light
{
	float3 positionWS;
	float padding;
	float3 color;
	float intensity;
};
#endif