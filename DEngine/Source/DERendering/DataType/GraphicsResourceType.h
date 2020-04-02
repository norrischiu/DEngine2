#pragma once

// Window
#include <d3d12.h>
// C++
#include <string>
// Engine
#include <DERendering/DataType/GraphicsNativeType.h>

using Microsoft::WRL::ComPtr;

namespace DE 
{

class Texture
{
public:
	Texture() = default;
	Texture(const Texture&) = default;
	~Texture() = default;

	void Init(ID3D12Resource* resource)
	{
		ptr = resource;
	}

	void Init(const GraphicsDevice& device, uint32_t width, uint32_t height, uint32_t depth, uint32_t mips, DXGI_FORMAT format, D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, const D3D12_CLEAR_VALUE* pClearValue = nullptr)
	{
		D3D12_HEAP_PROPERTIES heapProp = {};
		heapProp.Type = heapType;
		heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProp.CreationNodeMask = 1;
		heapProp.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.DepthOrArraySize = depth;
		desc.MipLevels = mips;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Format = format;
		desc.Alignment = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.Flags = flags;

		HRESULT hr = {};
		hr = device.ptr->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			initState,
			pClearValue,
			IID_PPV_ARGS(&ptr));
		assert(hr == S_OK);

		m_Desc = desc;
	}

	ComPtr<ID3D12Resource> ptr;
	D3D12_RESOURCE_DESC m_Desc;
	uint32_t m_iNumSubresources;
};

struct Buffer
{
public:

	Buffer() = default;
	Buffer(const Buffer&) = default;
	~Buffer() = default;

	void Init(const GraphicsDevice& device, std::size_t size, D3D12_HEAP_TYPE heapType)
	{
		D3D12_HEAP_PROPERTIES heapProp = {};
		heapProp.Type = heapType;
		heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProp.CreationNodeMask = 1;
		heapProp.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC desc = {};
		desc.Width = size;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.Alignment = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.SampleDesc.Count = 1;

		HRESULT hr = {};
		hr = device.ptr->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&ptr));
		assert(hr == S_OK);

		m_Desc = desc;
	}

	void Update(const void* memory, const size_t size, size_t offset = 0)
	{
		void* address = nullptr;

		D3D12_RANGE range{ 0, 0 };
		ptr->Map(0, &range, &address);
		memcpy((void*)(reinterpret_cast<uintptr_t>(address) + offset), memory, size);
		ptr->Unmap(0, &range);
	}

	void* Map(const size_t size = 0)
	{
		void* address = nullptr;
		D3D12_RANGE range{ 0, size };
		ptr->Map(0, &range, &address);
		return address;
	}

	void Unmap(const size_t size = 0)
	{
		D3D12_RANGE range{ 0, size };
		ptr->Unmap(0, &range);
	}

	ComPtr<ID3D12Resource> ptr;
	D3D12_RESOURCE_DESC m_Desc;
};

struct VertexBuffer final : public Buffer
{
	void Init(const GraphicsDevice& device, uint32_t stride, uint32_t size)
	{
		Buffer::Init(device, size, D3D12_HEAP_TYPE_UPLOAD);

		view.BufferLocation = ptr->GetGPUVirtualAddress();
		view.SizeInBytes = size;
		view.StrideInBytes = stride;
	}

	D3D12_VERTEX_BUFFER_VIEW view;
};

struct IndexBuffer final : public Buffer
{
	void Init(const GraphicsDevice& device, uint32_t stride, uint32_t size)
	{
		Buffer::Init(device, size, D3D12_HEAP_TYPE_UPLOAD);

		view.BufferLocation = ptr->GetGPUVirtualAddress();
		view.SizeInBytes = size;
		view.Format = stride == sizeof(uint32_t) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
	}
	D3D12_INDEX_BUFFER_VIEW view;
};

// for srv
struct ReadOnlyResourceDefinition
{
	uint32_t baseRegister;
	uint32_t num;
	D3D12_SHADER_VISIBILITY visibility;
	uint32_t rootParameterIndex;
};

// for uav
struct ReadWriteResourceDefinition
{
	uint32_t baseRegister;
	uint32_t num;
	D3D12_SHADER_VISIBILITY visibility;
	uint32_t rootParameterIndex;
};

// for cbv (because only as root parameter)
struct ConstantDefinition
{
	uint32_t baseRegister;
	D3D12_SHADER_VISIBILITY visibility;
	uint32_t rootParameterIndex;
};

// for sample (because only as static sampler)
struct SamplerDefinition
{
	uint32_t baseRegister;
	D3D12_SHADER_VISIBILITY visibility;
	D3D12_TEXTURE_ADDRESS_MODE addressMode;
	D3D12_FILTER filter;
	D3D12_COMPARISON_FUNC comparsionFunc;
	float maxLOD = D3D12_FLOAT32_MAX;
};

class RootSignature final
{
public:
	RootSignature() = default;
	RootSignature(const RootSignature&) = delete;
	RootSignature& operator=(const RootSignature&) = delete;
	~RootSignature()
	{
		ptr->Release();
	}

	void Add(ReadOnlyResourceDefinition* defs, uint32_t num)
	{
		for (uint32_t i = 0; i < num; ++i)
		{
			readOnlyResourceDefs[readOnlyResourceNum + i] = defs[i];
		}
		readOnlyResourceNum += num;
	}

	void Add(ConstantDefinition* defs, uint32_t num)
	{
		for (uint32_t i = 0; i < num; ++i)
		{
			constantDefs[constantNum + i] = defs[i];
		}
		constantNum += num;
	}

	void Add(SamplerDefinition* defs, uint32_t num)
	{
		for (uint32_t i = 0; i < num; ++i)
		{
			samplerDefs[samplerNum + i] = defs[i];
		}
		samplerNum += num;
	}

	void Finalize(const GraphicsDevice& device)
	{
		D3D12_ROOT_SIGNATURE_DESC desc;
		D3D12_ROOT_PARAMETER rootParameters[16];
		D3D12_DESCRIPTOR_RANGE range[16];
		D3D12_STATIC_SAMPLER_DESC samplers[4];
		uint32_t index = 0;

		assert(readOnlyResourceNum <= 16);

		for (uint32_t i = 0; i < readOnlyResourceNum; ++i)
		{
			readOnlyResourceDefs[i].rootParameterIndex = index;

			rootParameters[index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameters[index].ShaderVisibility = readOnlyResourceDefs[i].visibility;
			range[i].NumDescriptors = readOnlyResourceDefs[i].num;
			range[i].BaseShaderRegister = readOnlyResourceDefs[i].baseRegister;
			range[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			range[i].RegisterSpace = 0;
			range[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			rootParameters[index].DescriptorTable.NumDescriptorRanges = 1;
			rootParameters[index].DescriptorTable.pDescriptorRanges = &range[i];
			index++;
		}

		for (uint32_t i = 0; i < constantNum; ++i)
		{
			constantDefs[i].rootParameterIndex = index;
			
			rootParameters[index].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParameters[index].ShaderVisibility = constantDefs[i].visibility;
			rootParameters[index].Descriptor.RegisterSpace = 0;
			rootParameters[index].Descriptor.ShaderRegister = constantDefs[i].baseRegister;
			index++;
		}


		for (uint32_t i = 0; i < samplerNum; ++i)
		{
			samplers[i].ShaderVisibility = samplerDefs[i].visibility;
			samplers[i].ShaderRegister = samplerDefs[i].baseRegister;
			samplers[i].RegisterSpace = 0;
			samplers[i].AddressU = samplerDefs[i].addressMode;
			samplers[i].AddressV = samplerDefs[i].addressMode;
			samplers[i].AddressW = samplerDefs[i].addressMode;
			samplers[i].Filter = samplerDefs[i].filter;
			samplers[i].MipLODBias = 0.0f;
			samplers[i].MaxAnisotropy = 1;
			samplers[i].ComparisonFunc = samplerDefs[i].comparsionFunc;
			samplers[i].MinLOD = 0;
			samplers[i].MaxLOD = samplerDefs[i].maxLOD;
		}
		desc.pStaticSamplers = samplers;
		desc.NumStaticSamplers = samplerNum;

		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		desc.NumParameters = index;
		desc.pParameters = rootParameters;

		Init(device, desc);
	}

	void Init(const GraphicsDevice& device, const D3D12_ROOT_SIGNATURE_DESC& desc)
	{
		ID3DBlob* signature = nullptr;
		HRESULT hr = {};
		hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
		assert(hr == S_OK);
		hr = device.ptr->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&ptr));
		assert(hr == S_OK);

		signature->Release();
	}

	ID3D12RootSignature* ptr;

	ReadOnlyResourceDefinition readOnlyResourceDefs[16];
	ReadWriteResourceDefinition readWriteResourceDefs[16];
	ConstantDefinition constantDefs[16];
	SamplerDefinition samplerDefs[4];

	uint32_t readOnlyResourceNum = 0;
	uint32_t readWriteResourceNum = 0;
	uint32_t constantNum = 0;
	uint32_t samplerNum = 0;
};
}
