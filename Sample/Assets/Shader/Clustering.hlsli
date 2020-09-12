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
	float2 resolution;
};

/** @brief Get z index in cluster [0, numSlice) */
uint GetDepthSlice(float z, float zNear, float zFar, uint numSlice)
{
	return (uint) floor(log(z) * numSlice / log(zFar / zNear) - numSlice * log(zNear) / log(zFar / zNear));
}

/** @brief Get index to cluster by global thread Id */
uint GlobalThreadIdToClusterLinearId(uint3 globalThreadId, ClusterInfo info)
{
	return globalThreadId.x
		+ globalThreadId.y * info.numCluster.x
		+ globalThreadId.z * info.numCluster.x * info.numCluster.y;
}

/** @brief Get index to clusters list by screen position */
uint ScreenPosToClusterLinearId(float4 screenPos, ClusterInfo info) {
	uint clusterZVal = GetDepthSlice(screenPos.z * screenPos.w, info.zNear, info.zFar, info.numSlice);

	uint3 clusters = uint3(uint2(screenPos.xy) / info.clusterSize, clusterZVal);
	uint clusterIndex = clusters.x 
		+ clusters.y * info.numCluster.x
		+ clusters.z * info.numCluster.x * info.numCluster.y;
	return clusterIndex;
}

/** @brief Get 3D index to clusters list by screen position */
uint3 ScreenPosToClusterId(float4 screenPos, ClusterInfo info) {
	uint clusterZVal = GetDepthSlice(screenPos.z * screenPos.w, info.zNear, info.zFar, info.numSlice);
	return uint3(uint2(screenPos.xy) / info.clusterSize, clusterZVal);
}

#endif // !_CLUSTERING_HLSLI_