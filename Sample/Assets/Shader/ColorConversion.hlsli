#ifndef COLOR_CONVERSION_HLSLI
#define COLOR_CONVERSION_HLSLI

// D3DX_FLOAT_to_SRGB
float3 RGBtoSRGB(float3 rgb)
{
	float3 sRGBLo = rgb * 12.92;
	float3 sRGBHi = (pow(rgb, 1.0f / 2.4f) * 1.055f) - 0.055f;
	float3 sRGB = (rgb <= 0.0031308f) ? sRGBLo : sRGBHi;
	return sRGB;
}

// uncharted 2 filmic tone mapping: https://www.gdcvault.com/play/1012351/Uncharted-2-HDR
float3 FilmicToneMappingToSRGB(float3 color)
{
	color = max(0, color - 0.004f);
	return (color * (6.2f * color + 0.5f)) / (color * (6.2f * color + 1.7f) + 0.06f);
}
float3 FilmicToneMapping(float3 color)
{
	color = ((color * (0.22f * color + 0.1f * 0.3f) + 0.2f * 0.01f) / (color * (0.22f * color + 0.3f) + 0.2f * 0.3f)) - 0.01f / 0.3f;
	float3 white = float3(11.2f, 11.2f, 11.2f);
	white = ((white * (0.22f * white + 0.1f * 0.3f) + 0.2f * 0.01f) / (white * (0.22f * white + 0.3f) + 0.2f * 0.3f)) - 0.01f / 0.3f;
	return color / white;
}

float3 ReinhardToneMapping(float3 color)
{
	return color / (color + 1.0f);
}

#endif