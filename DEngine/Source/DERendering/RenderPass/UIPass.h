#pragma once

#include <DERendering/DataType/GraphicsNativeType.h>
#include <DERendering/DataType/GraphicsResourceType.h>

namespace DE
{

class RenderDevice;
class DrawCommandList;
class FrameData;

class UIPass
{
public:
	struct Data final
	{
	};

	UIPass() = default;

	void Setup(RenderDevice* renderDevice, const Data& data);
	void Execute(DrawCommandList& commandList, const FrameData& frameData);

private:
	Data m_data;
	RenderDevice* m_pDevice;

	Texture m_font;
	VertexBuffer m_vertexBuffer;
	IndexBuffer m_indexBuffer;
	GraphicsPipelineState m_pso;
	RootSignature m_rootSignature;

	uint32_t m_backBufferIndex = 0; // temp
};

} // namespace DE