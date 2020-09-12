#include <DERendering/DataType/GraphicsResourceType.h>
#include <DERendering/Device/RenderDevice.h>
#include "DrawCommandList.h"

namespace DE
{

uint32_t DrawCommandList::Init(RenderDevice* device)
{
	m_CommandList.Init(device->m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pDevice = device;

	D3D12_INDIRECT_ARGUMENT_DESC argDesc = {};
	argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
	D3D12_COMMAND_SIGNATURE_DESC desc = {};
	desc.ByteStride = sizeof(uint32_t) * 3;
	desc.NumArgumentDescs = 1;
	desc.pArgumentDescs = &argDesc;
	m_pDevice->m_Device.ptr->CreateCommandSignature(&desc, nullptr, IID_PPV_ARGS(&m_dispatchIndirectCmdSgn));

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
	scissorRect.left = static_cast<int64_t>(x);
	scissorRect.top = static_cast<int64_t>(y);
	scissorRect.right = static_cast<int64_t>(x + width);
	scissorRect.bottom = static_cast<int64_t>(y + height);
	m_CommandList.ptr->RSSetScissorRects(1, &scissorRect);
}

void DrawCommandList::SetScissorRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
	D3D12_RECT scissorRect;
	scissorRect.left = left;
	scissorRect.top = top;
	scissorRect.right = right;
	scissorRect.bottom = bottom;
	m_CommandList.ptr->RSSetScissorRects(1, &scissorRect);
}

void DrawCommandList::SetVertexBuffers(const VertexBuffer* buffers, uint32_t num)
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
	if (signature->type == RootSignature::Type::Graphics)
	{
		m_CommandList.ptr->SetGraphicsRootSignature(signature->ptr);
	}
	else // Compute
	{
		m_CommandList.ptr->SetComputeRootSignature(signature->ptr);
	}
}

void DrawCommandList::SetPipeline(const GraphicsPipelineState& pipeline)
{
	m_CommandList.ptr->SetPipelineState(pipeline.ptr);
}

void DrawCommandList::SetPipeline(const ComputePipelineState & pipeline)
{
	m_CommandList.ptr->SetPipelineState(pipeline.ptr);
}

void DrawCommandList::SetConstantResource(uint32_t index, const ConstantResource& resource)
{
	assert(m_pCurrSignature);
	assert(index < m_pCurrSignature->constantNum);

	const uint32_t rootParamterIndex = m_pCurrSignature->constantDefs[index].rootParameterIndex;
	if (m_pCurrSignature->type == RootSignature::Type::Graphics)
	{
		m_CommandList.ptr->SetGraphicsRootConstantBufferView(rootParamterIndex, resource.buffer.ptr->GetGPUVirtualAddress() + resource.buffer.offset + resource.offset);

	}
	else // Compute
	{
		m_CommandList.ptr->SetComputeRootConstantBufferView(rootParamterIndex, resource.buffer.ptr->GetGPUVirtualAddress() + resource.buffer.offset + resource.offset);
	}

}

void DrawCommandList::SetReadOnlyResource(uint32_t index, ReadOnlyResource* resources, uint32_t num)
{
	assert(m_pCurrSignature);
	assert(index < m_pCurrSignature->readOnlyResourceNum);

	const uint32_t rootParamterIndex = m_pCurrSignature->readOnlyResourceDefs[index].rootParameterIndex;
	D3D12_GPU_DESCRIPTOR_HANDLE descriptorTable = m_ShaderResourceSubHeap.gpuCurrent;
	for (uint32_t i = 0; i < num; ++i)
	{
		ShaderResourceView srv;
		srv.Init(m_pDevice->m_Device, m_ShaderResourceSubHeap, resources[i]);
	}
	if (m_pCurrSignature->type == RootSignature::Type::Graphics)
	{
		m_CommandList.ptr->SetGraphicsRootDescriptorTable(rootParamterIndex, descriptorTable);
	}
	else // Compute
	{
		m_CommandList.ptr->SetComputeRootDescriptorTable(rootParamterIndex, descriptorTable);
	}
}

void DrawCommandList::SetReadWriteResource(uint32_t index, ReadWriteResource* resources, uint32_t num)
{
	assert(m_pCurrSignature);
	assert(index < m_pCurrSignature->readWriteResourceNum);

	const uint32_t rootParamterIndex = m_pCurrSignature->readWriteResourceDefs[index].rootParameterIndex;
	D3D12_GPU_DESCRIPTOR_HANDLE descriptorTable = m_ShaderResourceSubHeap.gpuCurrent;
	for (uint32_t i = 0; i < num; ++i)
	{
		UnorderedAccessView uav;
		uav.Init(m_pDevice->m_Device, m_ShaderResourceSubHeap, resources[i]);
	}
	if (m_pCurrSignature->type == RootSignature::Type::Graphics)
	{
		m_CommandList.ptr->SetGraphicsRootDescriptorTable(rootParamterIndex, descriptorTable);
	}
	else // Compute
	{
		m_CommandList.ptr->SetComputeRootDescriptorTable(rootParamterIndex, descriptorTable);
	}
}

void DrawCommandList::SetRenderTargetDepth(RenderTargetView::Desc* renderTarget, uint32_t num, Texture* depth, uint32_t flag /*= 0*/, float* clearColor /*= nullptr*/, float clearDepth /*= 0.0f*/)
{
	D3D12_CPU_DESCRIPTOR_HANDLE descriptors = m_RtvSubHeap.current;
	for (uint32_t i = 0; i < num; ++i)
	{
		RenderTargetView::Allocate(m_pDevice->m_Device, m_RtvSubHeap, renderTarget[i]);
		if (flag & ClearFlag::Color) 
		{
			m_CommandList.ptr->ClearRenderTargetView(descriptors, clearColor, 0, nullptr);
		}
	}
	if (depth) 
	{
		D3D12_CPU_DESCRIPTOR_HANDLE depthDescriptor = m_DsvSubHeap.current;
		DepthStencilView dsv;
		dsv.Init(m_pDevice->m_Device, m_DsvSubHeap, *depth);
		if (flag & ClearFlag::Depth) 
		{
			m_CommandList.ptr->ClearDepthStencilView(depthDescriptor, D3D12_CLEAR_FLAG_DEPTH, clearDepth, 0, 0, nullptr);
		}
		
		m_CommandList.ptr->OMSetRenderTargets(num, &descriptors, true, &depthDescriptor);
	}
	else 
	{
		m_CommandList.ptr->OMSetRenderTargets(num, &descriptors, true, nullptr);
	}
}

void DrawCommandList::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset, uint32_t vertexOffset, uint32_t instanceOffset)
{
	if (!m_Barriers.empty())
	{
		m_CommandList.ptr->ResourceBarrier(m_Barriers.size(), m_Barriers.data());
		m_Barriers.clear();
	}

	m_CommandList.ptr->DrawIndexedInstanced(indexCount, instanceCount, indexOffset, vertexOffset, instanceOffset);
}

void DrawCommandList::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset, uint32_t instanceOffset)
{
	if (!m_Barriers.empty())
	{
		m_CommandList.ptr->ResourceBarrier(m_Barriers.size(), m_Barriers.data());
		m_Barriers.clear();
	}

	m_CommandList.ptr->DrawInstanced(vertexCount, instanceCount, vertexOffset, instanceOffset);
}

void DrawCommandList::DispatchIndirect(const Buffer & buffer)
{
	if (!m_Barriers.empty())
	{
		m_CommandList.ptr->ResourceBarrier(m_Barriers.size(), m_Barriers.data());
		m_Barriers.clear();
	}

	m_CommandList.ptr->ExecuteIndirect(m_dispatchIndirectCmdSgn.Get(), 1, buffer.ptr.Get(), 0, nullptr, 0);
}

void DrawCommandList::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
	if (!m_Barriers.empty())
	{
		m_CommandList.ptr->ResourceBarrier(m_Barriers.size(), m_Barriers.data());
		m_Barriers.clear();
	}

	m_CommandList.ptr->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void DrawCommandList::UnorderedAccessBarrier(const Texture & texture)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.UAV.pResource = texture.ptr.Get();

	m_CommandList.ptr->ResourceBarrier(1, &barrier);
}

void DrawCommandList::UnorderedAccessBarrier(const Buffer & buffer)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.UAV.pResource = buffer.ptr.Get();

	m_CommandList.ptr->ResourceBarrier(1, &barrier);
}

void DrawCommandList::ResourceBarrier(const Texture& texture, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = texture.ptr.Get();
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;

	m_CommandList.ptr->ResourceBarrier(1, &barrier);
}

void DrawCommandList::ResourceBarrier(const Buffer & buffer, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = buffer.ptr.Get();
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;

	m_CommandList.ptr->ResourceBarrier(1, &barrier);
}

};