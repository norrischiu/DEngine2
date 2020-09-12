#pragma once

// Windows
#include <d3d12.h>
// Engine
#include <DERendering/DERendering.h>
#include <DERendering/DataType/GraphicsViewType.h>
#include <DECore/Container/Vector.h>

namespace DE
{

class RenderDevice;

enum ClearFlag
{
	Color = 0x1,
	Depth = 0x2,
};

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
	void SetPipeline(const ComputePipelineState& pipeline);
	void SetViewportAndScissorRect(float x, float y, float width, float height, float zMin, float zMax);
	void SetScissorRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);

	void SetVertexBuffers(const VertexBuffer* buffers, uint32_t num);
	void SetIndexBuffer(const IndexBuffer& buffer);

	void SetConstantResource(uint32_t index, const ConstantResource& resource);
	void SetReadOnlyResource(uint32_t index, ReadOnlyResource* resources, uint32_t num);
	void SetReadWriteResource(uint32_t index, ReadWriteResource* resources, uint32_t num);
	void SetRenderTargetDepth(RenderTargetView::Desc* renderTarget, uint32_t num, Texture* depth, uint32_t flag = 0, float* clearColor = nullptr, float clearDepth = 0.0f);

	void UnorderedAccessBarrier(const Texture& texture);
	void UnorderedAccessBarrier(const Buffer& buffer);
	void ResourceBarrier(const Texture& texture, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
	void ResourceBarrier(const Buffer& buffer, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

	void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset, uint32_t vertexOffset, uint32_t instanceOffset);
	void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset, uint32_t instanceOffset);

	void DispatchIndirect(const Buffer& buffer);
	void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);

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
	Vector<D3D12_RESOURCE_BARRIER> m_Barriers;
	ComPtr<ID3D12CommandSignature> m_dispatchIndirectCmdSgn;
};

}