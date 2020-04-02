#pragma once

#include <DERendering/DERendering.h>
#include <DERendering/FrameData/MaterialMeshBatcher.h>
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
	Vector3 cameraPos;
	Matrix4 cameraView;
	Matrix4 cameraProjection;
	Matrix4 cameraWVP;
};

} // namespace DE