#pragma once

// Window
#include <d3d12.h>
// C++
#include <string>

namespace DE 
{

struct Texture
{
public:
	Texture() = default;
	virtual ~Texture()
	{
		if (ptr) 
		{
			ptr->Release();
		}
	}

	void Init(ID3D12Resource* resource)
	{
		ptr = resource;
	}

	void Init(const GraphicsDevice& device, uint32_t width, uint32_t height, uint32_t depth, uint32_t mips, DXGI_FORMAT format, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, const D3D12_CLEAR_VALUE* pClearValue = nullptr)
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

	ID3D12Resource* ptr;
	D3D12_RESOURCE_DESC m_Desc;
	uint32_t m_iNumSubresources;
};

struct Buffer
{
public:

	Buffer() = default;
	~Buffer()
	{
		if (ptr)
		{
			ptr->Release();
		}
	}

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

	ID3D12Resource* ptr;
	D3D12_RESOURCE_DESC m_Desc;
};

struct VertexBuffer final : public Buffer
{
	void Init(const GraphicsDevice& device, uint32_t stride, std::size_t size)
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
	void Init(const GraphicsDevice& device, uint32_t stride, std::size_t size)
	{
		Buffer::Init(device, size, D3D12_HEAP_TYPE_UPLOAD);

		view.BufferLocation = ptr->GetGPUVirtualAddress();
		view.SizeInBytes = size;
		view.Format = stride / 3 == sizeof(uint32_t) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
	}
	D3D12_INDEX_BUFFER_VIEW view;
};

struct RenderTarget final : public Texture
{
	D3D12_CPU_DESCRIPTOR_HANDLE m_pDescriptor;
};

}
