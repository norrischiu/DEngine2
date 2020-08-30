#ifndef _CLUSTERING_HLSLI_
#define _CLUSTERING_HLSLI_

struct Cluster
{
	float4 min;
	float4 max;
};

struct ClusterLightInfo
{
	uint index;
	uint numPointLight;
	uint numQuadLight;
};

struct ClusterInfo
{
	float zNear;
	float zFar;
	uint clusterSize;
	uint numSlice;
	uint2 numCluster;
	uint2 resolution;
};

uint GetDepthSlice(float z, float zNear, float zFar, uint numSlice)
{
	return (uint) floor(log(z) * numSlice / log(zFar / zNear) - numSlice * log(zNear) / log(zFar / zNear));
}

/** @brief Get index to clusters list by screen position */
uint GetClusterLinearId(float4 screenPos, ClusterInfo info) {
	uint clusterZVal = GetDepthSlice(screenPos.z * screenPos.w, info.zNear, info.zFar, info.numSlice);

	uint3 clusters = uint3(uint2(screenPos.xy) / info.clusterSize, clusterZVal);
	uint clusterIndex = clusters.x + info.numCluster.x * clusters.y + info.numCluster.x * info.numCluster.y * clusters.z;
	return clusterIndex;
}

/** @brief Get 3D index to clusters list by screen position */
uint3 GetClusterId(float4 screenPos, ClusterInfo info) {
	uint clusterZVal = GetDepthSlice(screenPos.z * screenPos.w, info.zNear, info.zFar, info.numSlice);
	return uint3(uint2(screenPos.xy) / info.clusterSize, clusterZVal);
}

#endif // !_CLUSTERING_HLSLI_