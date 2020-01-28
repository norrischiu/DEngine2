// RenderDevice.h: the class for Direct3D 12 renderer
#pragma once

// Windows
#include <Windows.h>
// Engine
#include <DECore/DECore.h>
#include <DERendering/DataType/GraphicsNativeType.h>
#include <DERendering/DataType/GraphicsResourceType.h>
#include <DERendering/Device/DescriptorHeapRing.h>
// C++
#include <mutex>

namespace DE
{

class Framegraph;
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

	/** @brief This Execute the command lists recorded in framegraph
	*
	*	@param framegraph
	*/
	void Render(const Framegraph& framegraph);

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

	/** @brief This function will execute the command lists recorded in framegraph
	*
	*	@param index index to the swapchain
	*	@return a weak pointer to the back buffer Texture object
	*/
	Texture* GetBackBuffer(uint32_t index);

	GraphicsInfrastructure		m_GraphicsInfra;
	GraphicsDevice				m_Device;
	DescriptorHeapRing			m_shaderResourceHeap;
	DescriptorHeapRing			m_RtvHeap;
	DescriptorHeapRing			m_DsvHeap;
	CommandQueue				m_RenderQueue;
	Fence						m_Fence;
	uint64_t					m_FenceValue;
	SwapChain					m_SwapChain;
	std::unique_ptr<Texture>	m_BackBuffers[2];

	std::mutex					m_mutex;
	Vector<ID3D12CommandList*>	m_ppCommandLists;
};

};