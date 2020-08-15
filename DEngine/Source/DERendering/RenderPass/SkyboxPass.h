#pragma once

// Engine
#include <DERendering/DataType/GraphicsNativeType.h>
#include <DERendering/DataType/GraphicsResourceType.h>

namespace DE
{

class RenderDevice;
class DrawCommandList;
class FrameData;

class SkyboxPass
{
public:
	struct Data final
	{
		Texture cubemap;
		Texture depth;
	};

	SkyboxPass() = default;

	void Setup(RenderDevice* renderDevice, const Data& data);
	void Execute(DrawCommandList& commandList, const FrameData& frameData);

private:
	Data m_data;
	RenderDevice* m_pDevice;

	uint32_t m_backBufferIndex = 0; // temp
	GraphicsPipelineState m_pso;
	RootSignature m_rootSignature;
};

} // namespace DE