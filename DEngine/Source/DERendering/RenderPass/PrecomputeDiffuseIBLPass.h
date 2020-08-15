#pragma once

// Engine
#include <DERendering/DataType/GraphicsNativeType.h>
#include <DERendering/DataType/GraphicsResourceType.h>

namespace DE
{

class RenderDevice;
class DrawCommandList;
class FrameData;

class PrecomputeDiffuseIBLPass
{
public:
	struct Data final
	{
		Texture equirectangularMap;
		Texture cubemap;
		Texture irradianceMap;
	};

	PrecomputeDiffuseIBLPass() = default;

	bool Setup(RenderDevice* renderDevice, const Data& data);
	void Execute(DrawCommandList& commandList, const FrameData& frameData);

private:
	Data m_data;

	VertexBuffer m_cubeVertices;
	GraphicsPipelineState m_pso;
	RootSignature m_rootSignature;

	GraphicsPipelineState m_convolutePso;
	RenderDevice* m_pDevice;
};

} // namespace DE