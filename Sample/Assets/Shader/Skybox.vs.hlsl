#pragma pack_matrix( row_major )

// gbuffer
cbuffer WVP : register(b0)
{
	matrix g_invProjection;
	matrix g_invView;
};

// output
struct VS_OUTPUT
{
	float4 vPos : SV_POSITION;
	float3 direction : TEXCOORD0;
};

// function
VS_OUTPUT main(uint id : SV_VERTEXID)
{
	VS_OUTPUT OUT;
	OUT.vPos.x = (float)(id / 2) * 4.0 - 1.0;
	OUT.vPos.y = (float)(id % 2) * 4.0 - 1.0;
	OUT.vPos.z = 1.0;
	OUT.vPos.w = 1.0;

	OUT.direction = mul(mul(OUT.vPos, g_invProjection).xyz, (float3x3)g_invView);

	return OUT;
}
