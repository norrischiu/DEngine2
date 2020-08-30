#pragma once

// Engine
#include <DERendering/DataType/GraphicsNativeType.h>
#include <DERendering/DataType/GraphicsResourceType.h>

namespace DE
{

class RenderDevice;
class DrawCommandList;
class FrameData;

class ForwardPass
{
public:
	struct Data final
	{
		Texture depth;
		Texture irradianceMap;
		Texture prefilteredEnvMap;
		Texture BRDFIntegrationMap;
		Texture LTCInverseMatrixMap;
		Texture LTCNormMap;
		Texture filteredAreaLightTexture;
		Buffer clusterLightInfoList;
		Buffer lightIndexList;
	};

	ForwardPass() = default;

	void Setup(RenderDevice* renderDevice, const Data& data);
	void Execute(DrawCommandList& commandList, const FrameData& frameData);

private:
	Data m_data;
	RenderDevice* m_pDevice;

	uint32_t m_backBufferIndex = 0;
	GraphicsPipelineState m_pso;
	GraphicsPipelineState m_noNormalMapPso;
	GraphicsPipelineState m_albedoOnlyPso;
	GraphicsPipelineState m_wireframePso;
	RootSignature m_rootSignature;
};

} // namespace DE