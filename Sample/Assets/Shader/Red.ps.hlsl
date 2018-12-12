struct VS_OUTPUT
{
	float4 vPos : SV_POSITION;
};

float4 main(VS_OUTPUT IN) : SV_TARGET
{
	return float4(1.0, 0.0, 0.0, 1.0);
}
