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

class EquirectangularToCubemapPass
{
	struct Data final
	{
		VertexBuffer cubeVertices;
		Texture src;
		Texture dst;

		ConstantBufferView cbvs[6];
		GraphicsPipelineState pso;
		RootSignature rootSignature;

		GraphicsPipelineState convolutePso;
		Texture irradianceMap;

		RenderDevice* pDevice;
	};

public:
	EquirectangularToCubemapPass() = default;

	bool Setup(RenderDevice* renderDevice, const Texture& equirectangularMap, Texture& cubemap, Texture& irradianceMap);
	void Execute(DrawCommandList& commandList, const FrameData& frameData);

private:
	Data data;
};

} // namespace DE