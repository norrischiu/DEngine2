#pragma once

#include <DERendering/DataType/GraphicsNativeType.h>
#include <DERendering/DataType/GraphicsViewType.h>
#include <DERendering/DataType/GraphicsResourceType.h>

namespace DE
{

class RenderDevice;
class DrawCommandList;
class FrameData;
class Texture;

class PrecomputeSpecularIBLPass
{
	struct Data final
	{
		uint32_t numRoughness = 5;
		ConstantBufferView prefilterData[5];
		ConstantBufferView LUTsize;

		VertexBuffer cubeVertices;
		Texture src;
		Texture dst;

		ConstantBufferView cbvs[6];
		GraphicsPipelineState pso;
		RootSignature rootSignature;

		GraphicsPipelineState convolutePso;
		Texture LUT;

		RenderDevice* pDevice;
	};

public:
	PrecomputeSpecularIBLPass() = default;

	bool Setup(RenderDevice* renderDevice, const Texture& cubemap, Texture& prefilteredEnvMap, Texture& LUT);
	void Execute(DrawCommandList& commandList, const FrameData& frameData);

private:
	Data data;
};

} // namespace DE