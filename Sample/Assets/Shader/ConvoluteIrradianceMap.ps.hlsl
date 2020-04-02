#pragma pack_matrix( row_major )

TextureCube inputTex : register(t0);
SamplerState inputSampler : register(s0);

struct VS_OUTPUT
{
	float4 vPos : SV_POSITION;
	float3 direction : TEXCOORD0;
};

const static float PI = 3.14159265359f;

float4 main(VS_OUTPUT IN) : SV_TARGET
{
	float3 normal = normalize(IN.direction);
	float3 irradiance = float3(0.0f, 0.0f, 0.0f);
	float3 right = cross(normal, float3(0.0f, 0.0f, 1.0f));
	float3 forward = cross(right, normal);

	float delta = 0.05f; // TODO: pass from cbuffer
	uint numSample = 0;

	for (float azimuth = 0.0f; azimuth < 2.0f * PI; azimuth += delta) // phi
	{
		for (float inclination = 0.0f; inclination < 0.5f * PI; inclination += delta) // theta
		{
			float3 direction = float3(sin(inclination) * cos(azimuth), cos(inclination), sin(inclination) * sin(azimuth)); // to range x:[-1,1], y:[-1,1], z:[-1,1]
			float3 sampleDirection = direction.x * right + direction.y * normal + direction.z * forward;
			float3 scaledIrradiance = inputTex.Sample(inputSampler, sampleDirection).rgb * cos(inclination) * sin(inclination);
			
			irradiance += scaledIrradiance;
			numSample++;
		}
	}

	return float4(irradiance * PI / (float)numSample, 1.0f);
}
