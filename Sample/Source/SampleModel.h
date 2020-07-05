#pragma once

#include <DEGame/Component/Camera.h>
#include <DEGame/Collection/Scene.h>
#include <DERendering/RenderPass/ForwardPass.h>
#include <DERendering/RenderPass/PrecomputeDiffuseIBLPass.h>
#include <DERendering/RenderPass/PrecomputeSpecularIBLPass.h>
#include <DERendering/RenderPass/PrefilterAreaLightTexturePass.h>
#include <DERendering/RenderPass/SkyboxPass.h>
#include <DERendering/RenderPass/UIPass.h>
#include <DERendering/DataType/GraphicsNativeType.h>
#include <DERendering/Device/DrawCommandList.h>
#include <DERendering/FrameData/FrameData.h>

namespace DE
{
class RenderDevice;
}

class SampleModel
{
public:
	SampleModel() = default;
	~SampleModel();

	void Setup(DE::RenderDevice* renderDevice);
	void Update(DE::RenderDevice& renderDevice, float dt);

private:
	void SetupUI();

	DE::Camera m_Camera;
	DE::DrawCommandList m_commandList;

	DE::ForwardPass m_forwardPass;
	DE::PrecomputeDiffuseIBLPass m_precomputeDiffuseIBLPass;
	DE::PrecomputeSpecularIBLPass m_precomputeSpecularIBLPass;
	DE::PrefilterAreaLightTexturePass m_prefilterAreaLightTexturePass;
	DE::SkyboxPass m_SkyboxPass;
	DE::UIPass m_UIPass;

	DE::Scene m_scene;
	DE::FrameData m_frameData;

	bool m_bFirstRun = true;
};