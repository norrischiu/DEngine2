#pragma once

// Engine
#include <DERendering/DataType/GraphicsNativeType.h>
#include <DERendering/DataType/GraphicsResourceType.h>

namespace DE
{

class RenderDevice;
class DrawCommandList;
class FrameData;

class PrefilterAreaLightTexturePass
{
public:
	struct Data final
	{
		Texture src;
		Texture dst;
	};

	PrefilterAreaLightTexturePass() = default;

	bool Setup(RenderDevice* renderDevice, const Data& data);
	void Execute(DrawCommandList& commandList, const FrameData& frameData);

private:
	Data m_data;
	RenderDevice* m_pDevice;

	ConstantBufferView m_cbv;
	ComputePipelineState m_pso;
	ComputePipelineState m_verticalPso;
	RootSignature m_rootSignature;
};

} // namespace DE