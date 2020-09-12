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

StructuredBuffer<Cluster> clusterList : register(t0);
StructuredBuffer<uint> visibleClusterIndices : register(t1);
StructuredBuffer<uint> visibleClusterCount : register(t2);
RWStructuredBuffer<uint> lightIndexList : register(u0);
RWStructuredBuffer<uint> lightIndexListCounter : register(u1);
RWStructuredBuffer<ClusterLightInfo> clusterLightInfoList : register(u2);

bool IntersectSphereAabb(float3 centerWorldSpace, float radius, float3 aabbMin, float3 aabbMax) {
	float3 center = mul(float4(centerWorldSpace, 1.0f), g_viewMatrix).xyz;
	float squaredDist = 0.0f;

	float3 closestPoint = clamp(center, aabbMin, aabbMax);
	float3 toCenter = center - closestPoint;
	float lengthSquared = dot(toCenter, toCenter);
	return lengthSquared <= (radius * radius);
}

#define DIM_X 64
#define DIM_Y 1
#define DIM_Z 1
[numthreads(DIM_X, DIM_Y, DIM_Z)]
void main(uint3 globalThreadId : SV_DispatchThreadID)
{
	if (globalThreadId.x >= visibleClusterCount[0])
	{
		return;
	}

	const uint clusterId = visibleClusterIndices[globalThreadId.x];
	const Cluster cluster = clusterList[clusterId];
	uint pointLightIndices[16], quadLightIndices[8];
	uint pointLightCount = 0, quadLightCount = 0;

	uint i = 0;
	for (; i < g_numPointLights; ++i)
	{
		PointLight pointLight = g_pointLights[i];
		if (IntersectSphereAabb(pointLight.positionWS, pointLight.falloffRadius, cluster.min.xyz, cluster.max.xyz))
		{
			pointLightIndices[pointLightCount++] = i;
		}
	}
	for (i = 0; i < g_numQuadLights; ++i)
	{
		QuadLight quadLight = g_quadLights[i];
		if (IntersectSphereAabb(quadLight.centerWS, quadLight.falloffRadius, cluster.min.xyz, cluster.max.xyz))
		{
			quadLightIndices[quadLightCount++] = i;
		}
	}

	uint index;
	InterlockedAdd(lightIndexListCounter[0], pointLightCount + quadLightCount, index);

	for (i = 0; i < pointLightCount; ++i)
	{
		lightIndexList[index + i] = pointLightIndices[i];
	}
	for (i = 0; i < quadLightCount; ++i)
	{
		lightIndexList[index + pointLightCount + i] = quadLightIndices[i];
	}
	clusterLightInfoList[clusterId].index = index;
	clusterLightInfoList[clusterId].numPointLight = pointLightCount;
	clusterLightInfoList[clusterId].numQuadLight = quadLightCount;
}