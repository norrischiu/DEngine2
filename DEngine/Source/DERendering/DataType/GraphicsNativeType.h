#pragma once

// Window
#include <d3d12.h>
#include <dxgi1_4.h>
#include <dxgidebug.h>
#include <wrl/client.h>
// Engine
#include <DERendering/DERendering.h>
// Cpp
#include <assert.h>

using Microsoft::WRL::ComPtr;

namespace DE
{

class GraphicsInfrastructure final
{
public:
	GraphicsInfrastructure() = default;
	GraphicsInfrastructure(const GraphicsInfrastructure&) = delete;
	GraphicsInfrastructure& operator=(const GraphicsInfrastructure&) = delete;
	~GraphicsInfrastructure()
	{
		ptr->Release();
#if DEBUG
		if (m_pDXGIDebug)
		{
			m_pDXGIDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
			m_pDXGIDebug->Release();
		}
		m_pDebugController->Release();
#endif
	}

	void Init(bool debug)
	{
		HRESULT hr = {};
		hr = CreateDXGIFactory1(IID_PPV_ARGS(&ptr));
		assert(hr == S_OK);
#if DEBUG
		if (debug) {
			hr = D3D12GetDebugInterface(IID_PPV_ARGS(&m_pDebugController));
			if (hr == S_OK)
			{
				m_pDebugController->EnableDebugLayer();
			}
			auto hModule = GetModuleHandle("Dxgidebug.dll");
			typedef HRESULT(*func)(REFIID, void**);
			func DXGIGetDebugInterface = (func)GetProcAddress(hModule, "DXGIGetDebugInterface");
			if (DXGIGetDebugInterface)
			{
				DXGIGetDebugInterface(IID_PPV_ARGS(&m_pDXGIDebug));
			}
		}
#endif
	}

	IDXGIFactory4* ptr;

private:
#if DEBUG
	ID3D12Debug* m_pDebugController; 
	IDXGIDebug* m_pDXGIDebug; 
#endif
};

class GraphicsDevice final
{
public:
	GraphicsDevice() = default;
	GraphicsDevice(const GraphicsDevice&) = delete;
	GraphicsDevice& operator=(const GraphicsDevice&) = delete;
	~GraphicsDevice()
	{
		ptr->Release();
	}

	void Init(void* adapter)
	{
		HRESULT hr = {};
		hr = D3D12CreateDevice(reinterpret_cast<IUnknown*>(adapter), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&ptr));
		assert(hr == S_OK);
	}

	ID3D12Device* ptr;
};

class CommandQueue final
{
public:
	CommandQueue() = default;
	CommandQueue(const CommandQueue&) = delete;
	CommandQueue(CommandQueue&&) = delete;
	CommandQueue& operator=(const CommandQueue&) = delete;
	CommandQueue& operator=(CommandQueue&&) = delete;
	~CommandQueue()
	{
		if (ptr)
		{
			ptr->Release();
		}
	}

	void Init(const GraphicsDevice& device, D3D12_COMMAND_LIST_TYPE type)
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.Type = type;

		HRESULT hr = {};
		hr = device.ptr->CreateCommandQueue(&desc, IID_PPV_ARGS(&ptr));
		assert(hr == S_OK);
	}

	ID3D12CommandQueue* ptr;
};

class SwapChain final
{
public:
	SwapChain() = default;
	SwapChain(const SwapChain&) = delete;
	SwapChain& operator=(const SwapChain&) = delete;
	~SwapChain()
	{
		for (uint32_t i = 0; i < backBufferCount; ++i)
		{
			if (backBuffers[i])
			{
				backBuffers[i]->Release();
			}
		}
		if (ptr)
		{
			ptr->Release();
			ptr = nullptr;
		}
		CloseHandle(waitable);
	}

	void Init(const GraphicsInfrastructure& graphicsInfra, const CommandQueue& commandQueue, HWND hWnd, uint32_t width, uint32_t height, uint32_t backBufferCount)
	{
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferCount = backBufferCount;
		swapChainDesc.BufferDesc.Width = width;
		swapChainDesc.BufferDesc.Height = height;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // TODO
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.OutputWindow = hWnd;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.Windowed = TRUE;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

		HRESULT hr = {};
		IDXGISwapChain* swapChainPtr;
		hr = graphicsInfra.ptr->CreateSwapChain(commandQueue.ptr, &swapChainDesc, &swapChainPtr);
		ptr = static_cast<IDXGISwapChain2*>(swapChainPtr);
		assert(hr == S_OK);

		for (uint32_t i = 0; i < backBufferCount; ++i)
		{
			ptr->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]));
		}

		this->backBufferCount = backBufferCount;

		ptr->SetMaximumFrameLatency(2);
		waitable = ptr->GetFrameLatencyWaitableObject();
	}

	IDXGISwapChain2* ptr;
	ID3D12Resource* backBuffers[4];
	uint32_t backBufferCount;
	HANDLE waitable;
};

class CommandList final
{
public:
	CommandList() = default;
	CommandList(const CommandList&) = delete;
	CommandList(CommandList&&) = delete;
	CommandList& operator=(CommandList&&) = delete;
	CommandList& operator=(const CommandList&) = delete;
	~CommandList()
	{
		if (ptr)
		{
			ptr->Release();
			ptr = nullptr;
		}
		if (m_Allocator)
		{
			m_Allocator->Release();
			m_Allocator = nullptr;
		}
	}

	void Init(const GraphicsDevice& device, D3D12_COMMAND_LIST_TYPE type)
	{
		HRESULT hr = {};
		hr = device.ptr->CreateCommandAllocator(type, IID_PPV_ARGS(&m_Allocator));
		assert(hr == S_OK);
		hr = device.ptr->CreateCommandList(0, type, m_Allocator, nullptr, IID_PPV_ARGS(&ptr));
		assert(hr == S_OK);

		ptr->Close();
	}

	void Start()
	{
		m_Allocator->Reset();
		ptr->Reset(m_Allocator, nullptr);
	}

	void End()
	{
		ptr->Close();
	}

	ID3D12GraphicsCommandList* ptr = nullptr;
private:
	ID3D12CommandAllocator* m_Allocator = nullptr;
};

class DescriptorHeap final
{
public:
	DescriptorHeap() = default;
	DescriptorHeap(const DescriptorHeap&) = default;
	DescriptorHeap& operator=(const DescriptorHeap&) = default;
	~DescriptorHeap()
	{
		ptr.Reset();
	}

	void Init(const GraphicsDevice& device, uint32_t descriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible)
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = descriptorCount;
		desc.Type = type;
		desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		HRESULT hr = {};
		hr = device.ptr->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&ptr));
		assert(hr == S_OK);

		current = ptr->GetCPUDescriptorHandleForHeapStart();
		gpuCurrent = ptr->GetGPUDescriptorHandleForHeapStart();
		stride = device.ptr->GetDescriptorHandleIncrementSize(type);
	}

	void Increment(uint32_t num)
	{
		current.ptr += stride * num;
		gpuCurrent.ptr += stride * num;
	}

	void Reset()
	{
		current = ptr->GetCPUDescriptorHandleForHeapStart();
		gpuCurrent = ptr->GetGPUDescriptorHandleForHeapStart();
	}

	ComPtr<ID3D12DescriptorHeap> ptr;
	D3D12_CPU_DESCRIPTOR_HANDLE current;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuCurrent;
	uint32_t stride;
};

class InputLayout final
{
public:
	InputLayout()
	{
		desc.pInputElementDescs = inputElementDesc;
	}
	InputLayout& operator=(const InputLayout&) = delete;
	~InputLayout() = default;
	
	static InputLayout Null()
	{
		InputLayout inputLayout;
		inputLayout.desc.pInputElementDescs = nullptr;
		inputLayout.desc.NumElements = 0;
		return inputLayout;
	}

	void Add(const char* semanticName, uint32_t semanticIndex, uint32_t inputSlot, DXGI_FORMAT format)
	{
		uint32_t i = desc.NumElements;
		inputElementDesc[i].SemanticName = semanticName;
		inputElementDesc[i].SemanticIndex = semanticIndex;
		inputElementDesc[i].InputSlot = inputSlot;
		inputElementDesc[i].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElementDesc[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		inputElementDesc[i].Format = format;
		inputElementDesc[i].InstanceDataStepRate = 0;

		desc.NumElements++;
	}

	D3D12_INPUT_LAYOUT_DESC desc = {};
	
private:
	D3D12_INPUT_ELEMENT_DESC inputElementDesc[16];
};

class GraphicsPipelineState final
{
public:
	GraphicsPipelineState() = default;
	GraphicsPipelineState(const GraphicsPipelineState&) = delete;
	GraphicsPipelineState& operator=(const GraphicsPipelineState&) = delete;
	~GraphicsPipelineState()
	{
		if (ptr) 
		{
			ptr->Release();
		}
	}

	void Init(const GraphicsDevice& device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
	{
		HRESULT hr = {};
		hr = device.ptr->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&ptr));
		assert(hr == S_OK);
	}

	ID3D12PipelineState* ptr;
};

class ComputePipelineState final
{
public:
	ComputePipelineState() = default;
	ComputePipelineState(const ComputePipelineState&) = delete;
	ComputePipelineState& operator=(const ComputePipelineState&) = delete;
	~ComputePipelineState()
	{
		if (ptr)
		{
			ptr->Release();
		}
	}

	void Init(const GraphicsDevice& device, const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc)
	{
		HRESULT hr = {};
		hr = device.ptr->CreateComputePipelineState(&desc, IID_PPV_ARGS(&ptr));
		assert(hr == S_OK);
	}

	ID3D12PipelineState* ptr;
};

class Fence final
{
public:
	Fence() = default;
	Fence(const Fence&) = delete;
	Fence& operator=(const Fence&) = delete;
	~Fence()
	{
		if (ptr)
		{
			ptr->Release();
		}
	}

	void Init(const GraphicsDevice& device, uint64_t initialValue = 0)
	{
		HRESULT hr = {};
		hr = device.ptr->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&ptr));
		assert(hr == S_OK);

		m_Event = CreateEvent(nullptr, false, false, nullptr);
	}

	void CPUWaitFor(uint64_t value)
	{
		if (ptr->GetCompletedValue() < value)
		{
			ptr->SetEventOnCompletion(value, m_Event);
			auto result = WaitForSingleObject(m_Event, INFINITE);
		}
	}

	ID3D12Fence* ptr;

private:
	HANDLE m_Event;
};

};