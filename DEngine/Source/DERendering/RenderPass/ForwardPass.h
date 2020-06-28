#pragma once

// Engine
#include <DERendering/Device/RenderDevice.h>
#include <DECore/Math/simdmath.h>

namespace DE
{

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
	RootSignature m_rootSignature;
	ConstantBufferView m_vsCbv;
	ConstantBufferView m_psCbv;
	ConstantBufferView m_materialCbv;
};

} // namespace DE