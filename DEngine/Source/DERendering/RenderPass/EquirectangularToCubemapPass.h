#pragma once

#include <DERendering/DERendering.h>

namespace DE
{

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