#pragma once

#include <DERendering/DERendering.h>
#include <DERendering/FrameData/MaterialMeshBatcher.h>
#include <DECore/Container/Vector.h>

namespace DE
{

class FrameData
{

	friend class ForwardPass;

public:
	FrameData() = default;

	MaterialMeshBatcher batcher;
	Matrix4 cameraView;
	Matrix4 cameraProjection;
	Matrix4 cameraWVP;
};

} // namespace DE