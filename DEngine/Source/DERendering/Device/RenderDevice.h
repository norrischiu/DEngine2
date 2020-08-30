// RenderDevice.h: the class for Direct3D 12 renderer
#pragma once

// Windows
#include <Windows.h>
// Engine
#include <DECore/DECore.h>
#include <DERendering/DataType/GraphicsNativeType.h>
#include <DERendering/DataType/GraphicsResourceType.h>
#include <DERendering/DataType/GraphicsViewType.h>
#include <DERendering/Device/DescriptorHeapRing.h>
#include <DERendering/Device/GpuBufferFencedPool.h>
#include <DECore/Container/Vector.h>
// C++
#include <mutex>

namespace DE
{

class CopyCommandList;
class DrawCommandList;

/** @brief Interface to D3D12 device, queue and swapchain*/
class RenderDevice
{

public:

	/** @brief Description to create a d3d12 renderer*/
	struct Desc final
	{
		HWND hWnd_;
		uint32_t windowHeight_;
		uint32_t windowWidth_;
		uint32_t backBufferCount_;
	};

	RenderDevice() = default;
	~RenderDevice();
	RenderDevice(const RenderDevice&) = delete;
	RenderDevice& operator=(const RenderDevice&) = delete;

	/** @brief	Initialize D3D12 object, e.g. queue, swapchain, fence
	*
	*	@param hWnd the window instance
	*	@return True if succeed, false if failed
	*/
	bool Init(const Desc& desc);

	/** @brief Reset any memory pointer or state */
	void Reset();

	/** @brief Submit command lists to a internal list
	*
	*	@param commandLists
	*	@param num
	*/
	void Submit(const CopyCommandList* commandLists, uint32_t num);
	void Submit(const DrawCommandList* commandLists, uint32_t num);

	/** @brief Execute the submitted command lists */
	void Execute();
	void ExecuteAndPresent();

	/** @brief Wait for render queue to be idle */
	void WaitForIdle();

	/** @brief Get back buffer
	*
	*	@param index index to the swapchain
	*	@return a pointer to the back buffer Texture object
	*/
	Texture* GetBackBuffer(uint32_t index);

	/** @brief Suballocate part of common upload buffer
	*
	*	@param required size
	*	@return a Buffer object
	*/
	Buffer SuballocateUploadBuffer(uint32_t size);

	GraphicsInfrastructure		m_GraphicsInfra;
	GraphicsDevice				m_Device;
	DescriptorHeapRing			m_shaderResourceHeap;
	DescriptorHeapRing			m_RtvHeap;
	DescriptorHeapRing			m_DsvHeap;
	CommandQueue				m_RenderQueue;
	Fence						m_Fence;
	uint64_t					m_FenceValue = 0;
	SwapChain					m_SwapChain;
	std::unique_ptr<Texture>	m_BackBuffers[2];

	GpuBufferFencedPool			m_UploadBufferPool;

	std::mutex					m_mutex;
	Vector<ID3D12CommandList*>	m_ppCommandLists;
};

};