#pragma once

#include <DERendering/DERendering.h>
#include <DERendering/Device/RenderDevice.h>

namespace DE
{

class DrawCommandList;
class FrameData;
class Texture;

class SkyboxPass
{
	struct Data final
	{
		Texture cubemap;

		ConstantBufferView cbv;
		GraphicsPipelineState pso;
		RootSignature rootSignature;
		Texture depth;

		uint32_t backBufferIndex = 0; // temp
		RenderDevice* pDevice;
	};

public:
	SkyboxPass() = default;

	void Setup(RenderDevice* renderDevice, const Texture& depth, const Texture& equirectangularMap);
	void Execute(DrawCommandList& commandList, const FrameData& frameData);

private:
	Data data;
};

} // namespace DE