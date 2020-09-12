#include "clustering.hlsli"

cbuffer ClusteringInfoCBuffer : register(b0)
{
	float4x4 g_perspectiveMatrixInverse;
	ClusterInfo g_clusterInfo;
};

Texture2D depth : register(t0);

RWStructuredBuffer<uint> visibleClusterIndices : register(u0);
RWStructuredBuffer<uint> visibleClusterCount : register(u1);
RWStructuredBuffer<uint> clusterVisibilities : register(u2);

#define DIM_X 8
#define DIM_Y 4
#define DIM_Z 8
[numthreads(DIM_X, DIM_Y, DIM_Z)]
void main(uint3 globalThreadId : SV_DispatchThreadID)
{
	const uint clusterId = GlobalThreadIdToClusterLinearId(globalThreadId, g_clusterInfo);
	if (clusterId >= g_clusterInfo.numCluster.x * g_clusterInfo.numCluster.y * g_clusterInfo.numSlice)
	{
		return;
	}

	if (clusterVisibilities[clusterId] > 0)
	{
		uint index;
		InterlockedAdd(visibleClusterCount[0], 1, index);
		visibleClusterIndices[index] = clusterId;
	}

	clusterVisibilities[clusterId] = 0;
}