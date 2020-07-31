// RenderDevice.cpp:

// Cpp
#include <assert.h>
// Engine
#include <DERendering/DERendering.h>
#include <DERendering/Device/CopyCommandList.h>
#include <DERendering/Device/DrawCommandList.h>
#include <DECore/Container/Vector.h>
#include "RenderDevice.h"

namespace DE 
{

RenderDevice::~RenderDevice()
{
	m_FenceValue++;

	CommandList finalCommandList;
	finalCommandList.Init(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	ID3D12CommandList* ppCommandLists[] = { finalCommandList.ptr };
	m_RenderQueue.ptr->ExecuteCommandLists(1, ppCommandLists);
	m_RenderQueue.ptr->Signal(m_Fence.ptr, m_FenceValue);

	m_Fence.CPUWaitFor(m_FenceValue);
	m_UploadBufferPool.Release();
}

bool RenderDevice::Init(const Desc& desc)
{
	m_GraphicsInfra.Init(true);

	IDXGIAdapter3* adapter = nullptr;
	int adapterItr = 0, adapterIndex = 0;
	size_t vram = 0;

	while (m_GraphicsInfra.ptr->EnumAdapters(adapterItr, reinterpret_cast<IDXGIAdapter**>(&adapter)) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			adapterItr++;
			continue;
		}
		if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
			// Query memory
			if (desc.DedicatedVideoMemory > vram)
			{
				vram = desc.DedicatedVideoMemory;
				adapterIndex = adapterItr;
			}
			adapter->Release();
		}
		adapterItr++;
	}
	m_GraphicsInfra.ptr->EnumAdapters(adapterIndex, reinterpret_cast<IDXGIAdapter**>(&adapter));
	assert(adapter);

	m_Device.Init(adapter);
	m_RenderQueue.Init(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_Fence.Init(m_Device);
	m_SwapChain.Init(m_GraphicsInfra, m_RenderQueue, desc.hWnd_, desc.windowWidth_, desc.windowHeight_, desc.backBufferCount_);
	m_shaderResourceHeap.Init(m_Device, 65536, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
	m_RtvHeap.Init(m_Device, 64, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);
	m_DsvHeap.Init(m_Device, 64, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false);

	for (uint32_t i = 0; i < desc.backBufferCount_; ++i)
	{
		m_BackBuffers[i] = std::make_unique<Texture>();
		m_BackBuffers[i]->Init(m_SwapChain.backBuffers[i]);
	}

	// common upload buffer
	m_UploadBufferPool.Init(this, D3D12_HEAP_TYPE_UPLOAD, 512 * 1024, 1);

	m_ppCommandLists.reserve(256);
	adapter->Release();

	return true;
}

void RenderDevice::Reset()
{
	m_shaderResourceHeap.Reset();
	m_RtvHeap.Reset();
	m_DsvHeap.Reset();

	m_UploadBufferPool.Reset(m_FenceValue);
}

void RenderDevice::Submit(const CopyCommandList* commandLists, uint32_t num)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (uint32_t cnt = 0; cnt < num; ++cnt)
	{
		HRESULT hr = commandLists[cnt].GetCommandList().ptr->Close();
		assert(hr == S_OK);
		m_ppCommandLists.push_back(static_cast<ID3D12CommandList*>(commandLists[cnt].GetCommandList().ptr));
	}
}

void RenderDevice::Submit(const DrawCommandList* commandLists, uint32_t num)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (uint32_t cnt = 0; cnt < num; ++cnt)
	{
		HRESULT hr = commandLists[cnt].GetCommandList().ptr->Close();
		assert(hr == S_OK);
		m_ppCommandLists.push_back(static_cast<ID3D12CommandList*>(commandLists[cnt].GetCommandList().ptr));
	}
}

void RenderDevice::Execute()
{
	m_RenderQueue.ptr->ExecuteCommandLists(m_ppCommandLists.size(), m_ppCommandLists.data());
	m_RenderQueue.ptr->Signal(m_Fence.ptr, m_FenceValue);

	m_ppCommandLists.clear();

	Reset();
}

void RenderDevice::ExecuteAndPresent()
{
	m_RenderQueue.ptr->ExecuteCommandLists(m_ppCommandLists.size(), m_ppCommandLists.data());
	m_RenderQueue.ptr->Signal(m_Fence.ptr, m_FenceValue);

	m_ppCommandLists.clear();

	m_SwapChain.ptr->Present(1, 0);

	Reset();
}

void RenderDevice::WaitForIdle()
{
	m_Fence.CPUWaitFor(m_FenceValue);
	m_FenceValue++;
}

Texture* RenderDevice::GetBackBuffer(uint32_t index)
{
	return m_BackBuffers[index].get();
}

Buffer RenderDevice::SuballocateUploadBuffer(uint32_t size)
{
	return m_UploadBufferPool.Allocate(m_FenceValue, size);
}

}