// input
struct VS_INPUT
{
	float3 vPos : POSITION;
};

// output
struct VS_OUTPUT
{
    float4 vPos : SV_POSITION;
};

// function
VS_OUTPUT main(VS_INPUT IN)
{
	VS_OUTPUT OUT;

	OUT.vPos = float4(IN.vPos, 1.0f);
	 
    return OUT;
}
