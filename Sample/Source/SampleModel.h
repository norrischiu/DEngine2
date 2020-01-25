#pragma once

#include <DEGame/Component/Camera.h>
#include <DEGame/Collection/Scene.h>
#include <DERendering/RenderPass/ForwardPass.h>
#include <DERendering/RenderPass/EquirectangularToCubemapPass.h>
#include <DERendering/RenderPass/SkyboxPass.h>
#include <DERendering/RenderPass/UIPass.h>
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
	void SetupUI();

	DE::Camera m_Camera;
	DE::DrawCommandList m_commandList;

	DE::ForwardPass m_forwardPass;
	DE::EquirectangularToCubemapPass m_precomputeCubemapPass;
	DE::SkyboxPass m_SkyboxPass;
	DE::UIPass m_UIPass;

	DE::Scene m_scene;
	DE::FrameData m_frameData;

	bool m_bFirstRun = true;
};