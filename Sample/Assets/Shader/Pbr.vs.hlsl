#pragma pack_matrix( row_major )

// cbuffer
cbuffer VS_PER_OBJECT : register(b0)
{
	matrix		g_WorldTransform;
	matrix		g_WVPTransform;
};

// input
struct VS_INPUT
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float2 texCoord : TEXCOORD;
};

// output
struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float4 posWS : TEXCOORD1;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float4 binormal : BINORMAL;
	float2 texCoord : TEXCOORD0;
};

// function
VS_OUTPUT main(VS_INPUT IN)
{
	VS_OUTPUT OUT;

	OUT.pos = mul(float4(IN.pos, 1.0f), g_WVPTransform);
	OUT.posWS = mul(float4(IN.pos, 1.0f), g_WorldTransform);
	OUT.normal = mul(float4(IN.normal, 0.0f), g_WorldTransform);
	OUT.tangent = mul(float4(IN.tangent, 0.0f), g_WorldTransform);
	OUT.binormal = mul(float4(normalize(cross(IN.normal, IN.tangent)), 0.0f), g_WorldTransform);
	OUT.texCoord = IN.texCoord;

    return OUT;
}
