#pragma pack_matrix( row_major )

// output
struct VS_OUTPUT
{
	float4 vPos : SV_POSITION;
};

// function
VS_OUTPUT main(uint id : SV_VERTEXID)
{
	VS_OUTPUT OUT;
	OUT.vPos.x = (float)(id / 2) * 4.0 - 1.0;
	OUT.vPos.y = (float)(id % 2) * 4.0 - 1.0;
	OUT.vPos.z = 1.0;
	OUT.vPos.w = 1.0;

	return OUT;
}
