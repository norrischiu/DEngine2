#pragma once

// Engine
#include <DERendering/Device/RenderDevice.h>
#include <DECore/Math/simdmath.h>

namespace DE
{

class DrawCommandList;
class FrameData;

class ForwardPass
{
	struct Data final
	{
		GraphicsPipelineState pso;
		GraphicsPipelineState texturedPso;
		RootSignature rootSignature;
		Texture depth;
		Texture irradianceMap;
		Texture prefilteredEnvMap;
		Texture LUT;
		uint32_t backBufferIndex = 0;

		ConstantBufferView vsCbv;
		ConstantBufferView psCbv;
		ConstantBufferView materialCbv;

		RenderDevice* pDevice;
	};

	struct PerLightCBuffer
	{
		Vector3 lightPos;
		Vector3 eyePos;
	};
	struct PerObjectCBuffer
	{
		Matrix4 world;
		Matrix4 wvp;
	};


public:
	ForwardPass() = default;

	void Setup(RenderDevice* renderDevice, Texture& irradianceMap, Texture& prefilteredEnvMap, Texture& LUT);
	void Execute(DrawCommandList& commandList, const FrameData& frameData);

	// temp
	Texture* GetDepth()
	{
		return &data.depth;
	}

private:
	Data data;
};

} // namespace DE