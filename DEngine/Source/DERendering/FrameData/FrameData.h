#pragma once

#include <DERendering/DERendering.h>
#include <DERendering/FrameData/MaterialMeshBatcher.h>
#include <DERendering/DataType/GraphicsDataType.h>
#include <DECore/Container/Vector.h>
#include <DECore/Math/simdmath.h>

namespace DE
{

class FrameData
{

	friend class ForwardPass;

public:
	FrameData() = default;

	MaterialMeshBatcher batcher;
	Vector<uint32_t> pointLights;
	Vector<uint32_t> quadLights;

	struct
	{
		Vector3 pos;
		Matrix4 view;
		Matrix4 projection;
		Matrix4 wvp;
		float zNear;
		float zFar;
	} camera;

	struct
	{
		uint32_t clusterSize;
		uint32_t numSlice;
		uint2 numCluster;
		float2 resolution;
	} clusteringInfo;
};

} // namespace DE