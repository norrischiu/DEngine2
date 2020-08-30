#pragma once

// Window
#include <d3d12.h>
// Cpp
#include <assert.h>
// Engine
#include <DERendering/DataType/GraphicsResourceType.h>

namespace DE
{

struct UnorderedAccessView
{
	void Init(const GraphicsDevice& device, DescriptorHeap& heap, const ReadWriteResource& resource)
	{
		ID3D12Resource* rawResource = nullptr;
		if (resource.dimension == D3D12_UAV_DIMENSION_TEXTURE2D)
		{
			desc.Texture2D.MipSlice = resource.mip;
			desc.Texture2D.PlaneSlice = 0;
			desc.Format = resource.texture.m_Desc.Format;
			rawResource = resource.texture.ptr.Get();
		} 
		else if (resource.dimension == D3D12_UAV_DIMENSION_BUFFER)
		{
			desc.Buffer.StructureByteStride = resource.stride;
			if (resource.numElement == 0)
			{
				desc.Buffer.NumElements = resource.buffer.m_Desc.Width / resource.stride;
			}
			else
			{
				desc.Buffer.NumElements = resource.numElement;
			}
			desc.Format = resource.buffer.m_Desc.Format;
			rawResource = resource.buffer.ptr.Get();
		}
		else 
		{
			assert(false); // not implemented
			return;
		}
		desc.ViewDimension = resource.dimension;

		descriptor = heap.current;
		device.ptr->CreateUnorderedAccessView(rawResource, nullptr, &desc, descriptor);
		heap.Increment(1);
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
	D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
};

struct ShaderResourceView
{
	void Init(const GraphicsDevice& device, DescriptorHeap& heap, const ReadOnlyResource& resource)
	{
		ID3D12Resource* rawResource = nullptr;
		if (resource.dimension == D3D12_SRV_DIMENSION_TEXTURECUBE)
		{
			desc.TextureCube.MipLevels = resource.texture.m_Desc.MipLevels;
			desc.TextureCube.MostDetailedMip = 0;
			desc.Format = resource.texture.ptr == nullptr ? DXGI_FORMAT_R8G8B8A8_UNORM : resource.texture.m_Desc.Format; // allow null descriptor
			rawResource = resource.texture.ptr.Get();
		}
		else if (resource.dimension == D3D12_SRV_DIMENSION_TEXTURE2D) 
		{
			if (resource.useTextureMipRange)
			{
				desc.Texture2D.MipLevels = resource.texture.m_Desc.MipLevels;
				desc.Texture2D.MostDetailedMip = 0;
			}
			else
			{
				desc.Texture2D.MipLevels = resource.numMips;
				desc.Texture2D.MostDetailedMip = resource.mip;
			}
			desc.Texture2D.PlaneSlice = 0;
			desc.Format = resource.texture.ptr == nullptr ? DXGI_FORMAT_R8G8B8A8_UNORM : resource.texture.m_Desc.Format; // allow null descriptor
			rawResource = resource.texture.ptr.Get();
		}
		else if (resource.dimension == D3D12_SRV_DIMENSION_BUFFER)
		{
			desc.Buffer.FirstElement = 0;
			if (resource.numElement == 0)
			{
				desc.Buffer.NumElements = resource.buffer.m_Desc.Width / resource.stride;
			}
			else
			{
				desc.Buffer.NumElements = resource.numElement;
			}
			desc.Buffer.StructureByteStride = resource.stride;
			desc.Format = resource.buffer.m_Desc.Format;
			rawResource = resource.buffer.ptr.Get();
		}
		else 
		{
			assert(false); // not implemented
			return;
		}
		desc.ViewDimension = resource.dimension;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		
		descriptor = heap.current;
		device.ptr->CreateShaderResourceView(rawResource, &desc, descriptor);
		heap.Increment(1);
	}	

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
};

struct RenderTargetView final
{
	struct Desc final
	{
		Texture* texture;
		uint32_t mip;
		uint32_t slice;
	};

	static void Allocate(const GraphicsDevice& device, DescriptorHeap& heap, const Desc& desc)
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.FirstArraySlice = desc.slice;
		rtvDesc.Texture2DArray.ArraySize = 1;
		rtvDesc.Texture2DArray.MipSlice = desc.mip;
		rtvDesc.Texture2DArray.PlaneSlice = 0;

		device.ptr->CreateRenderTargetView(desc.texture->ptr.Get(), &rtvDesc, heap.current);
		heap.Increment(1);
	}

#if 0
	void InitFromTexture2DArray(const GraphicsDevice& device, DescriptorHeap& heap, const Texture& texture, uint32_t arraySlice, uint32_t arraySize, uint32_t slice = 0, uint32_t mip = 0)
	{
		rtvDesc.Format = texture.m_Desc.Format;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.FirstArraySlice = arraySlice;
		rtvDesc.Texture2DArray.ArraySize = arraySize;
		rtvDesc.Texture2DArray.MipSlice = mip;
		rtvDesc.Texture2DArray.PlaneSlice = slice;

		descriptor = heap.current;
		device.ptr->CreateRenderTargetView(texture.ptr.Get(), &rtvDesc, descriptor);
		heap.current.ptr += device.ptr->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		resource = texture.ptr.Get();
	}
#endif

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
		device.ptr->CreateDepthStencilView(texture.ptr.Get(), &desc, descriptor);
		heap.current.ptr += device.ptr->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		resource = texture.ptr.Get();
	}

	ID3D12Resource* resource;
	D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
	D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
};

}