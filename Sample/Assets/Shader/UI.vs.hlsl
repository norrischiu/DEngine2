#pragma pack_matrix( row_major )

cbuffer Transform : register(b0)
{
	matrix g_projection;
};

// input
struct VS_INPUT
{
	float2 pos : POSITION;
	float4 color : COLOR0;
	float2 uv : TEXCOORD0;
};

// output
struct VS_OUTPUT
{
	float4 vPos : SV_POSITION;
	float4 color : COLOR0;
	float2 uv : TEXCOORD0;
};

// function
VS_OUTPUT main(VS_INPUT IN)
{
	VS_OUTPUT OUT;
	OUT.vPos = mul(float4(IN.pos, 0.0f, 1.0f), g_projection);
	OUT.color = IN.color;
	OUT.uv = IN.uv;

	return OUT;
}
