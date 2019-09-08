#include <DERendering/DERendering.h>
#include <DERendering/DataType/GraphicsResourceType.h>
#include "DrawCommandList.h"

namespace DE
{

uint32_t DrawCommandList::Init(RenderDevice& device)
{
	m_CommandList.Init(device.m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pDevice = &device;
	return 0;
}

void DrawCommandList::Begin()
{
	m_CommandList.Start();
	m_ShaderResourceSubHeap = m_pDevice->m_shaderResourceHeap.Alloc(1024);
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

void DrawCommandList::SetSignature(RootSignature& signature)
{
	m_pCurrSignature = &signature;
	m_CommandList.ptr->SetGraphicsRootSignature(signature.ptr);
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

void DrawCommandList::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset, uint32_t vertexOffset, uint32_t instanceOffset)
{
	for (const auto& barrier : m_Barriers) {
		m_CommandList.ptr->ResourceBarrier(1, &barrier);
	}
	m_Barriers.clear();

	m_CommandList.ptr->DrawIndexedInstanced(indexCount, instanceCount, indexOffset, vertexOffset, instanceOffset);
}

void DrawCommandList::ResourceBarrier(const RenderTargetView & rtv, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = rtv.resource;
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;

	m_CommandList.ptr->ResourceBarrier(1, &barrier);
}

};