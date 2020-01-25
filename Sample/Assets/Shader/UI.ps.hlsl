Texture2D inputTex : register(t0);
SamplerState inputSampler : register(s0);

struct VS_OUTPUT
{
	float4 vPos : SV_POSITION;
	float4 color : COLOR0;
	float2 uv : TEXCOORD0;
};

float4 main(VS_OUTPUT IN) : SV_TARGET
{
	return IN.color * inputTex.Sample(inputSampler, IN.uv);
}
