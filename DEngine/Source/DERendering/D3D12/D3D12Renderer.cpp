// D3D12Renderer.cpp:

// Windows
#include <d3d12.h>
#include <dxgi1_4.h>
// Engine
#include <DERendering/DERendering.h>
#include <DERendering/DataType/graphicsNativeType.h>
#include <DERendering/Framegraph/Framegraph.h>
#include "D3D12Renderer.h"

namespace DE 
{

D3D12Renderer::~D3D12Renderer()
{
	m_FenceValue++;

	CommandList finalCommandList;
	finalCommandList.Init(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	ID3D12CommandList* ppCommandLists[] = { finalCommandList.ptr };
	m_RenderQueue.ptr->ExecuteCommandLists(1, ppCommandLists);
	m_RenderQueue.ptr->Signal(m_Fence.ptr, m_FenceValue);

	m_Fence.CPUWaitFor(m_FenceValue);
}

bool D3D12Renderer::Init(const Desc& desc)
{
	m_GraphicsInfra.Init();

	IDXGIAdapter3* adapter = nullptr;
	int adapterIndex = 0;

	while (m_GraphicsInfra.ptr->EnumAdapters(adapterIndex, reinterpret_cast<IDXGIAdapter**>(&adapter)) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			adapter = nullptr;
			adapterIndex++;
			continue;
		}
		if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
		adapterIndex++;
	}
	assert(adapter);

	m_Device.Init(adapter);
	m_RenderQueue.Init(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_Fence.Init(m_Device);
	m_SwapChain.Init(m_GraphicsInfra, m_RenderQueue, desc.hWnd_, desc.windowWidth_, desc.windowHeight_, desc.backBufferCount_);

	for (uint32_t i = 0; i < desc.backBufferCount_; ++i)
	{
		ID3D12Resource* backBuffer = nullptr;
		m_SwapChain.ptr->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
		m_BackBuffers[i] = std::make_shared<Texture>();
		m_BackBuffers[i]->Init(backBuffer);
	}

	return true;
}

void D3D12Renderer::Render(const Framegraph& framegraph)
{
	const uint32_t commandListCount = static_cast<uint32_t>(framegraph.m_CommandLists.size());
	Vector<ID3D12CommandList*> ppCommandLists(commandListCount);
	for (uint32_t cnt = 0; cnt < commandListCount; ++cnt)
	{
		ppCommandLists[cnt] = framegraph.m_CommandLists[cnt]->ptr;
	}
	m_RenderQueue.ptr->ExecuteCommandLists(commandListCount, ppCommandLists.data());
	m_RenderQueue.ptr->Signal(m_Fence.ptr, m_FenceValue);

	m_SwapChain.ptr->Present(0, 0);

	m_Fence.CPUWaitFor(m_FenceValue);
	m_FenceValue++;
}

std::shared_ptr<Texture> D3D12Renderer::GetBackBuffer(uint32_t index)
{
	return m_BackBuffers[index];
}

}