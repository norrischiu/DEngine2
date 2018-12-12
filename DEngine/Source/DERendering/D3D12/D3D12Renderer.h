// D3D12Renderer.h: the class for Direct3D 12 renderer
#pragma once

// Windows
#include <Windows.h>
// Engine
#include <DECore/DECore.h>
#include <DERendering/DERendering.h>
#include <DERendering/DataType/GraphicsNativeType.h>
#include <DERendering/DataType/GraphicsResourceType.h>

namespace DE
{

class Framegraph;

/** @brief Interface to D3D12 device, queue and swapchain*/
class D3D12Renderer
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

	D3D12Renderer() = default;
	~D3D12Renderer();
	D3D12Renderer(const D3D12Renderer&) = delete;
	D3D12Renderer& operator=(const D3D12Renderer&) = delete;

	/** @brief	Initialize D3D12 object, e.g. queue, swapchain, fence
	*
	*	@param hWnd the window instance
	*	@return True if succeed, false if failed
	*/
	bool Init(const Desc& desc);

	/** @brief This function will execute the command lists recorded in framegraph
	*
	*	@param framegraph
	*/
	void Render(const Framegraph& framegraph);

	/** @brief This function will execute the command lists recorded in framegraph
	*
	*	@param index index to the swapchain
	*	@return the unique pointer to the back buffer Texture object
	*/
	std::shared_ptr<Texture> GetBackBuffer(uint32_t index);

	GraphicsInfrastructure		m_GraphicsInfra;
	GraphicsDevice				m_Device;
	CommandQueue				m_RenderQueue;
	Fence						m_Fence;
	uint64_t					m_FenceValue;
	SwapChain					m_SwapChain;
	std::shared_ptr<Texture>	m_BackBuffers[2];
};

};