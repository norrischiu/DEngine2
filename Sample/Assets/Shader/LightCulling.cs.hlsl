#include "Light.hlsli"
#include "Clustering.hlsli"
#pragma pack_matrix( row_major )

cbuffer ClusteringInfoCBuffer : register(b0)
{
	float4x4 g_perspectiveMatrixInverse;
	uint g_tileSize;
	float g_zNear;
	float g_zFar;
	uint g_numSlice;
	uint2 g_resolution;
	uint2 g_numCluster;
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
RWStructuredBuffer<uint> lightIndexList : register(u0);
RWStructuredBuffer<uint> lightIndexListCounter : register(u1);
RWStructuredBuffer<ClusterLightInfo> clusterLightInfoList : register(u2);

bool IntersectSphereAabb(float3 centerWorldSpace, float radius, float4 aabbMin, float4 aabbMax) {
	float3 center = mul(float4(centerWorldSpace, 1.0f), g_viewMatrix).xyz;
	float squaredDist = 0.0f;

	for (int i = 0; i < 3; ++i) {
		float v = center[i];
		if (v < aabbMin[i]) {
			squaredDist += (aabbMin[i] - v) * (aabbMin[i] - v);
		}
		if (v > aabbMax[i]) {
			squaredDist += (v - aabbMax[i]) * (v - aabbMax[i]);
		}
	}

	return squaredDist <= (radius * radius);
}

#define DIM_X 1
#define DIM_Y 1
#define DIM_Z 1
#define DIM_XYZ 1
[numthreads(DIM_X, DIM_Y, DIM_Z)]
void main(
	uint3 globalThreadId : SV_DispatchThreadID,
	uint3 groupId : SV_GroupId,
	uint localFlattenedId : SV_GroupIndex)
{
	if (globalThreadId.x == 0 && globalThreadId.y == 0 && globalThreadId.z == 0)
	{
		lightIndexListCounter[0] = 0;
	}

	uint pointLightIndices[16], quadLightIndices[8];
	uint pointLightCount = 0, quadLightCount = 0;
	const uint clusterId = groupId.x
		+ groupId.y * g_numCluster.x
		+ groupId.z * g_numCluster.x * g_numCluster.y;

	const Cluster cluster = clusterList[clusterId];

	uint i = 0;
	for (; i < g_numPointLights; ++i)
	{
		PointLight pointLight = g_pointLights[i];
		if (IntersectSphereAabb(pointLight.positionWS, pointLight.falloffRadius, cluster.min, cluster.max))
		{
			pointLightIndices[pointLightCount++] = i;
		}
	}
	for (i = 0; i < g_numQuadLights; ++i)
	{
		QuadLight quadLight = g_quadLights[i];
		if (IntersectSphereAabb(quadLight.centerWS, quadLight.falloffRadius, cluster.min, cluster.max))
		{
			quadLightIndices[quadLightCount++] = i;
		}
	}

	DeviceMemoryBarrierWithGroupSync();

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