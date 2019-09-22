#pragma once

#include <d3d12.h>
#include <DERendering/DERendering.h>


namespace DE
{

class DrawCommandList
{
public:

	DrawCommandList() = default;
	~DrawCommandList() = default;

	uint32_t Init(RenderDevice* device);

	void Begin();

	void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology);
	void SetSignature(RootSignature* signature);
	void SetPipeline(const GraphicsPipelineState& pipeline);
	void SetViewportAndScissorRect(float x, float y, float width, float height, float zMin, float zMax);
	
	void SetVertexBuffers(VertexBuffer* buffers, uint32_t num);
	void SetIndexBuffer(const IndexBuffer& buffer);

	void SetConstant(uint32_t index, const ConstantBufferView& cbv);
	void SetReadOnlyResource(uint32_t index, Texture* textures, uint32_t num);
	void SetRenderTargetDepth(Texture* renderTarget, uint32_t num, float* clearColor, Texture* depth, float clearDepth);

	void ResourceBarrier(const Texture& texture, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

	void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset, uint32_t vertexOffset, uint32_t instanceOffset);

	inline const CommandList& GetCommandList() const
	{
		return m_CommandList;
	}

//private:
	CommandList m_CommandList;
	DescriptorHeap m_ShaderResourceSubHeap;
	DescriptorHeap m_RtvSubHeap;
	DescriptorHeap m_DsvSubHeap;
	RenderDevice* m_pDevice;
	GraphicsPipelineState* m_pCurrPipeline;
	RootSignature* m_pCurrSignature;
	std::vector<D3D12_RESOURCE_BARRIER> m_Barriers;
};

}