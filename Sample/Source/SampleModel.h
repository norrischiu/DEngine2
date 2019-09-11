#pragma once

#include <DEGame/Component/Camera.h>
#include <DEGame/Collection/Scene.h>
#include <DERendering/RenderPass/ForwardPass.h>
#include <DERendering/DataType/GraphicsNativeType.h>
#include <DERendering/Device/DrawCommandList.h>
#include <DERendering/FrameData/FrameData.h>

namespace DE
{
class RenderDevice;
class Framegraph;
}

class SampleModel
{
public:
	SampleModel() = default;
	~SampleModel() = default;

	void Setup(DE::RenderDevice* renderDevice);
	void Update(DE::RenderDevice& renderDevice, float dt);

private:
	DE::Camera m_Camera;
	DE::DrawCommandList m_commandList;
	DE::ForwardPass m_forwardPass;

	DE::Scene m_scene;
	DE::FrameData m_frameData;
};