#pragma pack_matrix( row_major )

// cbuffer
cbuffer VS_PER_OBJECT : register(b0)
{
	matrix g_WorldTransform;
	matrix g_WVPTransform;
};

// input
struct VS_INPUT
{
	float3 pos : POSITION;
};

// output
struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
};

// function
VS_OUTPUT main(VS_INPUT IN)
{
	VS_OUTPUT OUT;

	OUT.pos = mul(float4(IN.pos, 1.0f), g_WVPTransform);

    return OUT;
}
