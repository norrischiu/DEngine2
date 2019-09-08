#pragma once

// Window
#include <d3d12.h>
#include <dxgi1_4.h>

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
		m_pDebugController->Release();
#endif
	}

	void Init(bool debug)
	{
		HRESULT hr = {};
#if DEBUG
		if (debug) {
			hr = D3D12GetDebugInterface(IID_PPV_ARGS(&m_pDebugController));
			if (hr == S_OK)
			{
				m_pDebugController->EnableDebugLayer();
			}
		}
#endif
		hr = CreateDXGIFactory1(IID_PPV_ARGS(&ptr));
		assert(hr == S_OK);
	}

	IDXGIFactory4* ptr;

private:
#if DEBUG
	ID3D12Debug* m_pDebugController;
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
private:
	IDXGIAdapter3* m_Adapter;
};

class CommandQueue final
{
public:
	CommandQueue() = default;
	CommandQueue(const CommandQueue&) = delete;
	CommandQueue& operator=(const CommandQueue&) = delete;
	~CommandQueue()
	{
		ptr->Release();
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
		if (ptr)
		{
			ptr->Release();
			ptr = nullptr;
		}
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

		HRESULT hr = {};
		hr = graphicsInfra.ptr->CreateSwapChain(commandQueue.ptr, &swapChainDesc, &ptr);
		assert(hr == S_OK);
	}

	IDXGISwapChain* ptr;
};

class CommandList final
{
public:
	CommandList() = default;
	CommandList(const CommandList&) = delete;
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

	ID3D12GraphicsCommandList* ptr;
private:
	ID3D12CommandAllocator* m_Allocator;
};

class DescriptorHeap final
{
public:
	DescriptorHeap() = default;
	DescriptorHeap(const DescriptorHeap&) = default;
	DescriptorHeap& operator=(const DescriptorHeap&) = default;
	~DescriptorHeap()
	{
		// ptr->Release();
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

	ID3D12DescriptorHeap* ptr;
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
	InputLayout(const InputLayout&) = delete;
	InputLayout& operator=(const InputLayout&) = delete;
	~InputLayout() = default;
	
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
		ptr->Release();
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
		ptr->Release();
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
		ptr->Release();
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
			WaitForSingleObject(m_Event, INFINITE);
		}
	}

	ID3D12Fence* ptr;

private:
	HANDLE m_Event;
};

};