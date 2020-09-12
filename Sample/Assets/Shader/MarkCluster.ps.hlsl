#include "Clustering.hlsli"

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
};

cbuffer ClusteringInfoCBuffer : register(b0)
{
	ClusterInfo g_clusterInfo;
};

RWStructuredBuffer<uint> clusterVisibilities : register(u0);

void main(VS_OUTPUT IN)
{
	uint clusterId = ScreenPosToClusterLinearId(IN.pos, g_clusterInfo);
	clusterVisibilities[clusterId] = 1;
}
