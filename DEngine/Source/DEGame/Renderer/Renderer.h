// Engine
#include <DEGame/Component/Camera.h>
#include <DEGame/Collection/Scene.h>
#include <DERendering/Device/RenderDevice.h>
#include <DERendering/RenderPass/ClusterLightPass.h>
#include <DERendering/RenderPass/ForwardPass.h>
#include <DERendering/RenderPass/PrecomputeDiffuseIBLPass.h>
#include <DERendering/RenderPass/PrecomputeSpecularIBLPass.h>
#include <DERendering/RenderPass/PrefilterAreaLightTexturePass.h>
#include <DERendering/RenderPass/SkyboxPass.h>
#include <DERendering/RenderPass/UIPass.h>
#include <DERendering/Device/DrawCommandList.h>
#include <DERendering/FrameData/FrameData.h>

namespace DE
{
class Renderer
{
public:
	/** @brief Description to create a d3d12 renderer*/
	struct Desc final
	{
		HWND hWnd;
		uint32_t windowHeight;
		uint32_t windowWidth;
		uint32_t backBufferCount;
	};

	Renderer() = default;
	~Renderer() = default;

	void Init(const Desc& desc);
	void Update(float dt);
	void Render();
	void Destruct();

	RenderDevice* GetRenderDevice()
	{
		return &m_RenderDevice;
	}

	Scene& GetScene()
	{
		return m_scene;
	}

private:
	Desc m_Desc;
	RenderDevice m_RenderDevice;

	Camera m_Camera;
	Scene m_scene;

	ClusterLightPass m_clusterLightPass;
	ForwardPass m_forwardPass;
	PrecomputeDiffuseIBLPass m_precomputeDiffuseIBLPass;
	PrecomputeSpecularIBLPass m_precomputeSpecularIBLPass;
	PrefilterAreaLightTexturePass m_prefilterAreaLightTexturePass;
	SkyboxPass m_SkyboxPass;
	UIPass m_UIPass;

	FrameData m_frameData;

	DrawCommandList m_commandList;

	bool m_bFirstRun = true;
};
}