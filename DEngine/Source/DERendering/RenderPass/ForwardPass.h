#pragma once

#include <DERendering/DERendering.h>

namespace DE
{

class DrawCommandList;
class FrameData;

class ForwardPass
{
	struct Data final
	{
		VertexBuffer position[10];
		VertexBuffer normal[10];
		VertexBuffer tangent[10];
		VertexBuffer texcoord[10];
		IndexBuffer ibs[10];
		GraphicsPipelineState pso;
		RootSignature rootSignature;
		RenderTargetView rtv[2];
		Texture depth;
		DepthStencilView dsv;
		ShaderResourceView srv[50];
		DescriptorHeap rtvDescriptorHeap;
		DescriptorHeap dsvDescriptorHeap;
		uint32_t backBufferIndex = 0;

		ConstantBufferView vsCbv;
		ConstantBufferView psCbv;

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

	void Setup(RenderDevice& renderDevice);
	void Execute(DrawCommandList& commandList, const FrameData& frameData);

private:
	Data data;
};

} // namespace DE