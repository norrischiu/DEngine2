#include "Light.hlsli"
#include "Clustering.hlsli"
#pragma pack_matrix( row_major )

cbuffer ClusteringInfoCBuffer : register(b0)
{
	float4x4 g_perspectiveMatrixInverse;
	ClusterInfo g_clusterInfo;
};

cbuffer PerViewCBuffer : register(b1)
{
	matrix g_viewMatrix;
	float3 g_eyePosWS;
	float padding;
	uint g_numPointLights;
	uint g_numQuadLights;
	float2 padding2;
	PointLight g_pointLights[8];
	QuadLight g_quadLights[8];
}

RWStructuredBuffer<Cluster> clusterList : register(u0);

float4 ScreenSpaceToViewSpace(float4 screenPos, float2 resolution)
{
	float2 uv = screenPos.xy / resolution;
	float4 posClipSpace = float4(float2(uv.x * 2.0f - 1.0f, 1.0f - 2.0f * uv.y), screenPos.z, screenPos.w);
	float4 posViewSpace = mul(posClipSpace, g_perspectiveMatrixInverse);
	return posViewSpace / posViewSpace.w;
}

float3 lineIntersectionToZPlane(float3 A, float3 B, float zDistance) {
	float3 normal = float3(0.0, 0.0, 1.0);
	float3 ab = B - A;
	float t = (zDistance - dot(normal, A)) / dot(normal, ab);
	return A + t * ab;
}

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

	float4 minScreenSpace = float4(globalThreadId.xy * g_clusterInfo.clusterSize, 0.0f, 1.0f);
	float4 maxScreenSpace = float4(float2(globalThreadId.x + 1, globalThreadId.y + 1) * g_clusterInfo.clusterSize, 0.0f, 1.0f);

	float4 minViewSpace = ScreenSpaceToViewSpace(minScreenSpace, g_clusterInfo.resolution);
	float4 maxViewSpace = ScreenSpaceToViewSpace(maxScreenSpace, g_clusterInfo.resolution);

	// Logarithmical depth distribution (THE DEVIL IS IN THE DETAILSIDTECH 666)
	float depthNear = g_clusterInfo.zNear * pow((g_clusterInfo.zFar / g_clusterInfo.zNear), globalThreadId.z / (float)g_clusterInfo.numSlice);
	float depthFar = g_clusterInfo.zNear * pow((g_clusterInfo.zFar / g_clusterInfo.zNear), (globalThreadId.z + 1) / (float)g_clusterInfo.numSlice);

	// Finding the 4 intersection points made from each point to the cluster near/far plane
	float3 zero = float3(0.0f, 0.0f, 0.0f);
	float3 minPointNear = lineIntersectionToZPlane(zero, minViewSpace.xyz, depthNear);
	float3 minPointFar = lineIntersectionToZPlane(zero, minViewSpace.xyz, depthFar);
	float3 maxPointNear = lineIntersectionToZPlane(zero, maxViewSpace.xyz, depthNear);
	float3 maxPointFar = lineIntersectionToZPlane(zero, maxViewSpace.xyz, depthFar);

	float3 minPointAABB = min(min(minPointNear, minPointFar), min(maxPointNear, maxPointFar));
	float3 maxPointAABB = max(max(minPointNear, minPointFar), max(maxPointNear, maxPointFar));

	clusterList[clusterId].min = float4(minPointAABB, 0.0f);
	clusterList[clusterId].max = float4(maxPointAABB, 0.0f);
}