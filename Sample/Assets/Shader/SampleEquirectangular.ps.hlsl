#pragma pack_matrix( row_major )

Texture2D inputTex : register(t0);
SamplerState inputSampler : register(s0);

struct VS_OUTPUT
{
	float4 vPos : SV_POSITION;
	float3 direction : TEXCOORD0;
};

float4 main(VS_OUTPUT IN) : SV_TARGET
{
	float3 direction = normalize(IN.direction);
	float2 uv = float2(atan2(direction.x, -direction.z), acos(direction.y)); // to spherical
	uv *= (1.0f / 3.14159265359f); // to range x:[-1,1], y:[0,1]
	uv.x = uv.x * 0.5 + 0.5f; // to range x:[0,1]
	return inputTex.Sample(inputSampler, uv);
}
