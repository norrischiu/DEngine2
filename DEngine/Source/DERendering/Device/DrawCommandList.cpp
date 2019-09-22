#include <DERendering/DERendering.h>
#include <DERendering/DataType/GraphicsResourceType.h>
#include "DrawCommandList.h"

namespace DE
{

uint32_t DrawCommandList::Init(RenderDevice* device)
{
	m_CommandList.Init(device->m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pDevice = device;
	return 0;
}

void DrawCommandList::Begin()
{
	m_CommandList.Start();
	m_ShaderResourceSubHeap = m_pDevice->m_shaderResourceHeap.Alloc(1024);
	m_RtvSubHeap = m_pDevice->m_RtvHeap.Alloc(16);
	m_DsvSubHeap = m_pDevice->m_DsvHeap.Alloc(16);

	ID3D12DescriptorHeap* heap[] = { m_pDevice->m_shaderResourceHeap.Raw() };
	m_CommandList.ptr->SetDescriptorHeaps(1, heap);
}

void DrawCommandList::SetViewportAndScissorRect(float x, float y, float width, float height, float zMin, float zMax)
{
	D3D12_VIEWPORT viewport;
	viewport.TopLeftX = x;
	viewport.TopLeftY = y;
	viewport.Width = width;
	viewport.Height = height;
	viewport.MinDepth = zMin;
	viewport.MaxDepth = zMax;
	m_CommandList.ptr->RSSetViewports(1, &viewport);
	
	D3D12_RECT scissorRect;
	scissorRect.left = x;
	scissorRect.top = y;
	scissorRect.right = x + width;
	scissorRect.bottom = y + height;
	m_CommandList.ptr->RSSetScissorRects(1, &scissorRect);
}

void DrawCommandList::SetVertexBuffers(VertexBuffer* buffers, uint32_t num)
{
	Vector<D3D12_VERTEX_BUFFER_VIEW> views(num);
	for (uint32_t i = 0; i < num; ++i)
	{
		views[i] = buffers[i].view;
	}
	m_CommandList.ptr->IASetVertexBuffers(0, num, views.data());

}

void DrawCommandList::SetIndexBuffer(const IndexBuffer& buffer)
{
	m_CommandList.ptr->IASetIndexBuffer(&buffer.view);
}

void DrawCommandList::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology)
{
	m_CommandList.ptr->IASetPrimitiveTopology(topology);
}

void DrawCommandList::SetSignature(RootSignature* signature)
{
	m_pCurrSignature = signature;
	m_CommandList.ptr->SetGraphicsRootSignature(signature->ptr);
}

void DrawCommandList::SetPipeline(const GraphicsPipelineState& pipeline)
{
	m_CommandList.ptr->SetPipelineState(pipeline.ptr);
}

void DrawCommandList::SetConstant(uint32_t index, const ConstantBufferView& cbv)
{
	assert(m_pCurrSignature);
	assert(index < m_pCurrSignature->constantNum);

	const uint32_t rootParamterIndex = m_pCurrSignature->constantDefs[index].rootParameterIndex;
	m_CommandList.ptr->SetGraphicsRootConstantBufferView(rootParamterIndex, cbv.buffer.ptr->GetGPUVirtualAddress());
}

void DrawCommandList::SetReadOnlyResource(uint32_t index, Texture* textures, uint32_t num)
{
	assert(m_pCurrSignature);
	assert(index < m_pCurrSignature->readOnlyResourceNum);

	const uint32_t rootParamterIndex = m_pCurrSignature->readOnlyResourceDefs[index].rootParameterIndex;
	D3D12_GPU_DESCRIPTOR_HANDLE descriptorTable = m_ShaderResourceSubHeap.gpuCurrent;
	for (uint32_t i = 0; i < num; ++i)
	{
		ShaderResourceView srv;
		srv.Init(m_pDevice->m_Device, m_ShaderResourceSubHeap, textures[i]);
	}
	m_CommandList.ptr->SetGraphicsRootDescriptorTable(rootParamterIndex, descriptorTable);
}

void DrawCommandList::SetRenderTargetDepth(Texture* renderTarget, uint32_t num, float* clearColor, Texture* depth, float clearDepth)
{
	D3D12_CPU_DESCRIPTOR_HANDLE descriptors = m_RtvSubHeap.current;
	for (uint32_t i = 0; i < num; ++i)
	{
		RenderTargetView rtv;
		rtv.Init(m_pDevice->m_Device, m_RtvSubHeap, renderTarget[i]);
		if (clearColor != nullptr) {
			m_CommandList.ptr->ClearRenderTargetView(descriptors, clearColor, 0, nullptr);
			clearColor += 4;
		}
	}
	if (depth) 
	{
		D3D12_CPU_DESCRIPTOR_HANDLE depthDescriptor = m_DsvSubHeap.current;
		DepthStencilView dsv;
		dsv.Init(m_pDevice->m_Device, m_DsvSubHeap, *depth);
		m_CommandList.ptr->ClearDepthStencilView(depthDescriptor, D3D12_CLEAR_FLAG_DEPTH, clearDepth, 0, 0, nullptr);
		
		m_CommandList.ptr->OMSetRenderTargets(num, &descriptors, true, &depthDescriptor);
	}
	else 
	{
		m_CommandList.ptr->OMSetRenderTargets(num, &descriptors, true, nullptr);
	}
}

void DrawCommandList::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset, uint32_t vertexOffset, uint32_t instanceOffset)
{
	for (const auto& barrier : m_Barriers) {
		m_CommandList.ptr->ResourceBarrier(1, &barrier);
	}
	m_Barriers.clear();

	m_CommandList.ptr->DrawIndexedInstanced(indexCount, instanceCount, indexOffset, vertexOffset, instanceOffset);
}

void DrawCommandList::ResourceBarrier(const Texture& texture, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture.ptr;
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;

	m_CommandList.ptr->ResourceBarrier(1, &barrier);
}

};