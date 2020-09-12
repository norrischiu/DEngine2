#pragma once

// Engine
#include <DERendering/DataType/GraphicsNativeType.h>
#include <DERendering/DataType/GraphicsResourceType.h>

namespace DE
{

class RenderDevice;
class DrawCommandList;
class FrameData;

class ZPrePass
{
public:
	struct Data final
	{
		Texture depth;
		Buffer clusterVisibilities;
	};

	ZPrePass() = default;

	void Setup(RenderDevice* renderDevice, const Data& data);
	void Execute(DrawCommandList& commandList, const FrameData& frameData);

private:
	Data m_data;
	RenderDevice* m_pDevice;

	GraphicsPipelineState m_pso;
	RootSignature m_rootSignature;
};

} // namespace DE