TextureCube inputTex : register(t0);
SamplerState inputSampler : register(s0);

struct VS_OUTPUT
{
	float4 vPos : SV_POSITION;
	float3 direction : TEXCOORD0;
};

float4 main(VS_OUTPUT IN) : SV_TARGET
{
	return inputTex.Sample(inputSampler, normalize(IN.direction));
}
