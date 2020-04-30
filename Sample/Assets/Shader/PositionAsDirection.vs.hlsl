#pragma pack_matrix( row_major )

cbuffer WVP : register(b0)
{
	matrix transform;
};

// input
struct VS_INPUT
{
	float3 vPos : POSITION;
};

// output
struct VS_OUTPUT
{
	float4 vPos : SV_POSITION;
	float3 direction : TEXCOORD0;
};

// function
VS_OUTPUT main(VS_INPUT IN)
{
	VS_OUTPUT OUT;

	OUT.direction = IN.vPos;
	OUT.vPos = mul(float4(IN.vPos, 1.0f), transform);

	return OUT;
}
