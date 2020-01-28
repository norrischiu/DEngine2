#pragma once

#include <DERendering/DERendering.h>
#include <DERendering/Device/RenderDevice.h>

namespace DE
{

class DrawCommandList;
class FrameData;
class Texture;

class UIPass
{
	struct Data final
	{
		Texture font;

		VertexBuffer vertexBuffer;
		IndexBuffer indexBuffer;
		ConstantBufferView cbv;
		GraphicsPipelineState pso;
		RootSignature rootSignature;

		uint32_t backBufferIndex = 0; // temp
		RenderDevice* pDevice;
	};

public:
	UIPass() = default;

	void Setup(RenderDevice* renderDevice);
	void Execute(DrawCommandList& commandList, const FrameData& frameData);

private:
	Data data;
};

} // namespace DE