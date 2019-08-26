#pragma once

// Window
#include <d3d12.h>
// Engine
#include <DERendering/DataType/GraphicsResourceType.h>

namespace DE
{

struct UnorderedAccessView
{
	ID3D12Resource* resource;
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc;
	D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
};

struct ShaderResourceView
{
	void Init(const GraphicsDevice& device, DescriptorHeap& heap, const Texture& texture)
	{
		desc.Texture2D.MipLevels = 1;
		desc.Texture2D.MostDetailedMip = 0;
		desc.Texture2D.PlaneSlice = 0;
		desc.Format = texture.m_Desc.Format;
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		
		assert(desc.Format != DXGI_FORMAT_UNKNOWN);

		descriptor = heap.current;
		device.ptr->CreateShaderResourceView(texture.ptr, &desc, descriptor);
		heap.current.ptr += device.ptr->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		resource = texture.ptr;
	}

	ID3D12Resource* resource;
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
};

struct ConstantBufferView
{
	void Init(const GraphicsDevice& device, std::size_t size)
	{
		buffer.Init(device, size, D3D12_HEAP_TYPE_UPLOAD);
	}

	Buffer buffer;
	D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
	D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
};

struct RenderTargetView final
{
	void Init(const GraphicsDevice& device, DescriptorHeap& heap, const Texture& texture, uint32_t mip = 0, uint32_t plane = 0)
	{
		desc.Format = texture.m_Desc.Format;
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = mip;
		desc.Texture2D.PlaneSlice = plane;

		descriptor = heap.current;
		device.ptr->CreateRenderTargetView(texture.ptr, &desc, descriptor);
		heap.current.ptr += device.ptr->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		resource = texture.ptr;
	}

	void InitFromTexture2DArray(const GraphicsDevice& device, DescriptorHeap& heap, const Texture& texture, uint32_t arraySlice, uint32_t arraySize, uint32_t slice = 0, uint32_t mip = 0)
	{
		desc.Format = texture.m_Desc.Format;
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.FirstArraySlice = arraySlice;
		desc.Texture2DArray.ArraySize = arraySize;
		desc.Texture2DArray.MipSlice = mip;
		desc.Texture2DArray.PlaneSlice = slice;

		descriptor = heap.current;
		device.ptr->CreateRenderTargetView(texture.ptr, &desc, descriptor);
		heap.current.ptr += device.ptr->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		resource = texture.ptr;
	}

	ID3D12Resource* resource;
	D3D12_RENDER_TARGET_VIEW_DESC desc = {};
	D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
};

struct DepthStencilView
{
	void Init(const GraphicsDevice& device, DescriptorHeap& heap, const Texture& texture)
	{
		desc.Format = texture.m_Desc.Format;
		desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;

		descriptor = heap.current;
		device.ptr->CreateDepthStencilView(texture.ptr, &desc, descriptor);
		heap.current.ptr += device.ptr->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		resource = texture.ptr;
	}

	ID3D12Resource* resource;
	D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
	D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
};

}