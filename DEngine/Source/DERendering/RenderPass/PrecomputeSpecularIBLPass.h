#pragma once

// Engine
#include <DERendering/DataType/GraphicsNativeType.h>
#include <DERendering/DataType/GraphicsResourceType.h>

namespace DE
{

class RenderDevice;
class DrawCommandList;
class FrameData;

class PrecomputeSpecularIBLPass
{
public:
	struct Data final
	{
		uint32_t numRoughness = 5;
		Texture cubemap;
		Texture prefilteredEnvMap;
		Texture LUT;
	};

	PrecomputeSpecularIBLPass() = default;

	bool Setup(RenderDevice* renderDevice, const Data& data);
	void Execute(DrawCommandList& commandList, const FrameData& frameData);

private:
	Data m_data;
	RenderDevice* m_pDevice;

	VertexBuffer m_cubeVertices;
	GraphicsPipelineState m_pso;
	RootSignature m_rootSignature;
	GraphicsPipelineState m_convolutePso;
};

} // namespace DE